class Timing
    attr_reader :count, :total, :min, :max

    def initialize
        @count = 0
        @total = 0.0
        @min = Float::INFINITY
        @max = -Float::INFINITY
    end

    def time
        start = Time.now
        yield
    ensure
        duration = Time.now - start
        add duration
    end

    def add(duration)
        @count += 1
        @total += duration
        @min = duration if duration < @min
        @max = duration if duration > @max
    end

    def mean
        @total / @count.to_f
    end
end