MRuby::Build.new do |conf|
  conf.toolchain
  conf.gembox 'default'
  conf.cc.flags += %w[-O3 -fPIC -s]
end
