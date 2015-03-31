$:.push File.expand_path("../lib", __FILE__)

if ENV['DEVELOPMENT']
  VERSION = `git describe --tags`.strip.gsub('-', '.')[1..-1]
else
  VERSION = "0.1.0"
end

Gem::Specification.new do |s|
  s.name                  = "rfreeimage"
  s.version               = VERSION
  s.date                  = Time.now.strftime('%Y-%m-%d')
  s.summary               = "Ruby extension for FreeImage library."
  s.homepage              = "https://github.com/chyh1990/rfreeimage"
  s.email                 = "chyh1990@gmail.com"
  s.authors               = [ "Yuheng Chen" ]
  s.license               = "MIT"
  s.files                 = %w( README.md )
  s.files                 += Dir.glob("lib/**/*.rb")
  s.files                 += Dir.glob("ext/**/*.[ch]")
  s.extensions            = ['ext/rfreeimage/extconf.rb']
  s.required_ruby_version = '>= 1.9.3'
  s.description           = <<desc
  RFreeImage is the ruby binding for FreeImage library.
desc
  s.add_development_dependency "rake-compiler", ">= 0.9.0"
  s.add_development_dependency "minitest", "~> 3.0.0"
end
