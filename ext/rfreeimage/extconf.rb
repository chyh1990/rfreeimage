require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"
$CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
$CFLAGS << " -Wall -Wno-comment"

def sys(cmd)
	puts " -- #{cmd}"
	unless ret = xsystem(cmd)
		raise "ERROR: '#{cmd}' failed"
	end
	ret
end

if !(MAKE = find_executable('gmake') || find_executable('make'))
	abort "ERROR: GNU make is required to build Rugged."
end

CWD = File.expand_path(File.dirname(__FILE__))
FREEIMAGE_DIR = File.join(CWD, '..', '..', 'vendor', 'FreeImage')

Dir.chdir(FREEIMAGE_DIR) do
	sys(MAKE)
end

$DEFLIBPATH.unshift("#{FREEIMAGE_DIR}/Dist")
dir_config('freeimage', "#{FREEIMAGE_DIR}/Dist", "#{FREEIMAGE_DIR}/Dist")

have_library('stdc++')
have_library('freeimage')

create_makefile("rfreeimage/rfreeimage")
