# frozen_string_literal: true

require 'test_helper'

class MinibwaTest < Test::Unit::TestCase
  test 'VERSION' do
    assert do
      ::Minibwa.const_defined?(:VERSION)
    end
  end

  test 'Error is a StandardError' do
    assert_operator(Minibwa::Error, :<, StandardError)
  end

  test 'public classes are defined' do
    %i[Options Index Buffer Hit].each do |name|
      assert(Minibwa.const_defined?(name), "expected Minibwa::#{name}")
    end
  end
end
