require 'rake/testtask'

begin
	require 'rake/extensiontask'
rescue LoadError
	abort <<-error
  rake-compile is missing; Rugged depends on rake-compiler to build the C wrapping code.
  Install it by running `gem i rake-compiler`
	error
end

gemspec = Gem::Specification::load(File.expand_path('../rfreeimage.gemspec', __FILE__))

Gem::PackageTask.new(gemspec) do |pkg|
end

Rake::ExtensionTask.new('rfreeimage', gemspec) do |r|
	r.lib_dir = 'lib/rfreeimage'
end

desc "checkout freeimage source"
task :checkout do
	if !ENV['CI_BUILD']
		sh "git submodule update --init"
	end
end
Rake::Task[:compile].prerequisites.insert(0, :checkout)

Rake::TestTask.new do |t|
	t.libs << 'lib' << 'test'
	t.pattern = 'test/*_test.rb'
	t.verbose = false
	t.warning = true
end
