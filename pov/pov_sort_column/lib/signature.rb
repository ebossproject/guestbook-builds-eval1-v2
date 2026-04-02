class Signature
  attr_accessor :name, :email, :message, :created_at_epoch

  def initialize
    self.name = slug()
    self.email = slug() + "@example.com"
    self.message = slug()
    self.created_at_epoch = Time.now.to_f
  end

  private

  def slug
    rand(36 ** 8).to_s(36)
  end
end
