require "logger"

LLL = Logger.new(STDERR,
                 formatter: proc do |sev, datetime, progname, msg|
                   "#{sev}: #{msg}\n"
                 end,
                 level: Logger::DEBUG)
