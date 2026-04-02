#!/usr/bin/env ruby

# Copyright (c) 2026 Cromulence
# range of lengths to pick in the absence of a `LENGTH` environment variable
LENGTH_RANGE = (100..200)

# seconds to wait for a given activity
DEFAULT_TIME_LIMIT = 5

require "fileutils"
require "mechanize"
require "nokogiri"
require "timeout"

require_relative "./lib/assertions"
require_relative "./lib/logger"
require_relative "./lib/signature"
require_relative "./lib/timing"

class Poller
  attr_reader :seed, :length

  attr_reader :host, :port

  attr_reader :report, :assertions

  ACTIVITIES = %i{
    index
    sign
    filter_sort
    filter_where
    filter_both
    schema
  }
  MAX_ACTIVITY_NAME_LEN = ACTIVITIES.map(&:to_s).map(&:length).max

  def initialize
    @host = ENV["HOST"] || "guestbook"
    @port = (ENV["PORT"] || 8080).to_i

    @base_url = "http://#{@host}:#{@port}"

    @report = Hash[ACTIVITIES.map { |activity| [activity, Timing.new] }]
    @assertions = 0

    @entries = []
    @preflight_saw_preexisting_state = false

    seed_rng!
    pick_length!
    pick_timeout!
    pick_focus!
  end

  def run!
    preflight!
    sign

    length.times do
      break if ACTIVITIES.empty?
      activity = ACTIVITIES.sample

      unless respond_to?(activity, true)
        LLL.error "activity #{activity} not implemented"
        ACTIVITIES.delete(activity)
        next
      end

      LLL.info "trying #{activity}"
      @report[activity].time do
        Timeout.timeout(@timeout) do
          send activity
        end
      end
    end
  end

  include Assertions

  def preflight!
    with_agent do |mech|
      begin
        index = mech.get("#{@base_url}/")
        assert_equal(200, index.code.to_i)
        assert_equal("text/html", index.response["content-type"])

        rows = index.search("table tbody tr")
        @preflight_saw_preexisting_state = true if rows.size > 0
      end
    end
  end

  def index
    with_agent do |mech|
      index = mech.get("#{@base_url}/")
      assert_equal(200, index.code.to_i)
      assert_equal("text/html", index.response["content-type"])

      if @preflight_saw_preexisting_state
        LLL.warn "preflight saw preexisting state, skipping guestbook contents checks"
        next
      end

      rows = index.search("table tbody tr")
      @entries.reverse.zip(rows.to_a).each do |entry, row|
        cells = row.css("td")
        assert cells[0].text.include?(entry.name), "couldn't find #{entry.inspect} name in #{cells[0].inspect}"
        assert cells[1].text.include?(entry.email), "couldn't find #{entry.inspect} email in #{cells[1].inspect}"
        assert cells[2].text.include?(entry.message), "couldn't find #{entry.inspect} message in #{cells[2].inspect}"
      end
    end
  end

  def sign
    with_agent do |mech|
      sign = mech.get("#{@base_url}/")
      assert_equal(200, sign.code.to_i)
      assert_equal("text/html", sign.response["content-type"])

      form = sign.form_with(action: "/sign")
      assert(form, "no sign form")

      signature = Signature.new

      form["name"] = signature.name
      form["email"] = signature.email
      form["message"] = signature.message

      @entries << signature

      sign = form.submit
      assert_equal("#{@base_url}/", sign.uri.to_s)
    end
  end

  def filter_sort
    sort_field = %i{name email message}.sample
    sort_order = %i{asc desc}.sample

    sort_proc = case sort_order
      when :asc
        ->(a, b) { a.send(sort_field) <=> b.send(sort_field) }
      when :desc
        ->(a, b) { b.send(sort_field) <=> a.send(sort_field) }
      end

    sort_param = "order_by[#{sort_field}]=#{sort_order}"

    uri = URI("#{@base_url}/filter?#{sort_param}")

    sorted_entries = @entries.sort(&sort_proc)

    with_agent do |mech|
      filtered = mech.get(uri)
      assert_equal(200, filtered.code.to_i)
      assert_equal("text/html", filtered.response["content-type"])

      if @preflight_saw_preexisting_state
        LLL.warn "preflight saw preexisting state, skipping guestbook contents checks"
        return
      end

      rows = filtered.search("table tbody tr")
      sorted_entries.zip(rows.to_a).each do |entry, row|
        cells = row.css("td")
        assert cells[0].text.include?(entry.name), "couldn't find #{entry.inspect} name in #{cells[0].inspect}"
        assert cells[1].text.include?(entry.email), "couldn't find #{entry.inspect} email in #{cells[1].inspect}"
        assert cells[2].text.include?(entry.message), "couldn't find #{entry.inspect} message in #{cells[2].inspect}"
      end
    end
  end

  def filter_where
    where_field = %i{name email message}.sample
    where_value = @entries.sample.send(where_field)

    where_param = "where[#{where_field}]=#{where_value}"

    uri = URI("#{@base_url}/filter?#{where_param}")

    filtered_entries = @entries.select { |entry| entry.send(where_field) == where_value }

    with_agent do |mech|
      filtered = mech.get(uri)
      assert_equal(200, filtered.code.to_i)
      assert_equal("text/html", filtered.response["content-type"])

      if @preflight_saw_preexisting_state
        LLL.warn "preflight saw preexisting state, skipping guestbook contents checks"
        return
      end

      rows = filtered.search("table tbody tr")
      filtered_entries.zip(rows.to_a).each do |entry, row|
        cells = row.css("td")
        assert cells[0].text.include?(entry.name), "couldn't find #{entry.inspect} name in #{cells[0].inspect}"
        assert cells[1].text.include?(entry.email), "couldn't find #{entry.inspect} email in #{cells[1].inspect}"
        assert cells[2].text.include?(entry.message), "couldn't find #{entry.inspect} message in #{cells[2].inspect}"
      end
    end
  end

  def filter_both
    sort_field = %i{name email message}.sample
    sort_order = %i{asc desc}.sample

    sort_proc = case sort_order
      when :asc
        ->(a, b) { a.send(sort_field) <=> b.send(sort_field) }
      when :desc
        ->(a, b) { b.send(sort_field) <=> a.send(sort_field) }
      end

    sort_param = "order_by[#{sort_field}]=#{sort_order}"

    sorted_entries = @entries.sort(&sort_proc)

    where_field = %i{name email message}.sample
    where_value = sorted_entries.sample.send(where_field)

    where_param = "where[#{where_field}]=#{where_value}"

    uri = URI("#{@base_url}/filter?#{sort_param}&#{where_param}")

    filtered_entries = sorted_entries.select { |entry| entry.send(where_field) == where_value }

    with_agent do |mech|
      filtered = mech.get(uri)
      assert_equal(200, filtered.code.to_i)
      assert_equal("text/html", filtered.response["content-type"])

      if @preflight_saw_preexisting_state
        LLL.warn "preflight saw preexisting state, skipping guestbook contents checks"
        return
      end

      rows = filtered.search("table tbody tr")
      filtered_entries.zip(rows.to_a).each do |entry, row|
        cells = row.css("td")
        assert cells[0].text.include?(entry.name), "couldn't find #{entry.inspect} name in #{cells[0].inspect}"
        assert cells[1].text.include?(entry.email), "couldn't find #{entry.inspect} email in #{cells[1].inspect}"
        assert cells[2].text.include?(entry.message), "couldn't find #{entry.inspect} message in #{cells[2].inspect}"
      end
    end
  end

  def schema
    with_agent do |mech|
      schema = mech.get("#{@base_url}/schema")
      assert_equal(200, schema.code.to_i)
      rows = schema.search("table tr")
      assert rows.size > 0, "no rows in schema table"
      name_columns = rows.css("td:nth-child(1)")
      assert name_columns.to_a.any? { |td| td.text == "guests" }, "no guests header in schema table"
    end
  end

  def report!
    $stderr.puts("%#{MAX_ACTIVITY_NAME_LEN}s\t%s\t%s\t%s\t%s" % %w{activity count min mean max})
    total_count = 0
    total_min = Float::INFINITY
    total_max = -1
    total_mean = 0
    @report.each do |activity, timing|
      total_count += timing.count
      total_min = timing.min if timing.min < total_min
      total_max = timing.max if timing.max > total_max
      total_mean += (timing.count * timing.mean)
      $stderr.puts("%#{MAX_ACTIVITY_NAME_LEN}s\t%d\t%.4f\t%.4f\t%.4f" % [activity, timing.count, timing.min, timing.mean, timing.max])
    end
    $stderr.puts("%#{MAX_ACTIVITY_NAME_LEN}s\t%d\t%.4f\t%.4f\t%.4f" % ["TOTAL", total_count, total_min, total_mean / total_count, total_max])
  end

  def snapshot!
    minute = 60

    s = Socket.new(Socket::AF_INET, Socket::SOCK_STREAM)
    
    begin
      Timeout.timeout(minute * 10) do
        return false unless connect_socket(s)
        return false unless manage_socket(s)
      end
    rescue Timeout::Error
      puts "Snapshot Connection Failure: connection failed"
      return false
    ensure
      s.close if s && !s.closed?
    end

    true
  end

  private

  def connect_socket(sock)
    status = false

    begin
      sock.connect(Addrinfo.tcp(@host, 9040))
    rescue Timeout::Error
      puts "Snapshot Connection Failure: connection failed"
    rescue => e
      puts "Snapshot Connection Failure: #{e}"
    else
      status = true
    end

    status
  end

  def manage_socket(sock)
    done = false
    fail = false

    begin
      sock.each_line do |line|
        if (done = line.include?("snapshot-complete"))
          puts "Flag 'snapshot-complete' received from socket"
          break
        end
        puts line.rstrip
      end
    rescue => e
      puts "Snapshot Socket Failure: #{e}"
      fail = true
    end

    return false if !done || fail

    true
  end

  # get the seed from either the `SEED` environment variable or from urandom
  def seed_rng!
    @seed = (ENV["SEED"] || Random.new_seed).to_i
    $stdout.puts "SEED=#{@seed}"
    $stdout.flush
    srand(@seed)
  end

  # get the length (number of activities) from either `LENGTH` environment
  # variable or by sampling a length from the `LENGTH_RANGE` constant
  def pick_length!
    @length = (ENV["LENGTH"] || rand(LENGTH_RANGE)).to_i
    $stdout.puts "LENGTH=#{@length}"
    $stdout.flush
  end

  # pick a timeout (seconds to wait per activity) from either `TIMEOUT`
  # environment variable or the `DEFAULT_TIME_LIMIT` constant
  def pick_timeout!
    @timeout = (ENV["TIMEOUT"] || DEFAULT_TIME_LIMIT).to_i
    $stdout.puts "TIMEOUT=#{@timeout}"
    $stdout.flush
  end

  def pick_focus!
    return unless ENV["OOPS_ALL"]

    focus = ENV["OOPS_ALL"].to_sym

    ACTIVITIES.keep_if { |e| e == focus }

    fail "Couldn't foucs on #{focus}" if ACTIVITIES.empty?
  end

  def with_agent
    Mechanize.start do |mech|
      begin
        mech.follow_redirect = true
        mech.agent.allowed_error_codes = [404, 425]
        yield mech
      rescue
        FileUtils.mkdir_p File.join(__dir__, "tmp")
        dump_filename = File.join(__dir__, "tmp",
                                  "#{Time.now.to_i}-s#{@seed}")

        File.open(dump_filename, "w") { |d| d.write mech.current_page.body }

        LLL.fatal dump_filename
        raise
      end
    end
  end
end

if $0 == __FILE__
  poller = Poller.new

  at_exit do
    LLL.info(["SEED=#{poller.seed}",
              "LENGTH=#{poller.length}",
              "ruby", $0, *ARGV].join(" "))
  end

  poller.run!
  poller.snapshot!
  poller.report!
end
