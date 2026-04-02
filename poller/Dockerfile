FROM ruby:3

RUN mkdir -p /app
WORKDIR /app

COPY Gemfile /app/Gemfile
RUN bundle install

COPY . /app

CMD ["./poller.rb"]
