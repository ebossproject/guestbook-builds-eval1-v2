module Assertions
  # assert two values are equal
  def assert_equal(expected, actual)
    if expected.is_a?(Nokogiri::XML::Node) && actual.is_a?(Nokogiri::XML::Node)
      assert(EquivalentXml.equivalent?(expected, actual,
                                       element_order: false, normalize_whitespace: true),
             "XML nodes were not equal")
    elsif expected.is_a?(String) && actual.is_a?(String)
      ex_bin = expected.dup.force_encoding("BINARY")
      act_bin = actual.dup.force_encoding("BINARY")

      assert(ex_bin == act_bin,
             "strings were not equal; expected #{ex_bin.inspect}, got #{act_bin.inspect}")
    else
      assert(expected == actual, "expected `#{expected}`, got `#{actual}`")
    end
  end

  # assert a predicate is true
  def assert(predicate, message = "failed assertion")
    @assertions += 1
    return if predicate
    fail message
  end

  def refute(predicate, message = "failed refutation")
    @assertions += 1
    return unless predicate
    fail message
  end
end
