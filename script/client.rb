require 'socket'

starttime = Process.clock_gettime(Process::CLOCK_MONOTONIC)

s = UNIXSocket.new("/home/francesco/Documenti/Unipi/Sistemi Operativi/Progetto/filestorageserver/socket/l.sock")

s.write("/tmp/TESTFILES/#{ARGV[0]}.txt\n")

s.each_line do |line| 
	#	puts line
end

s.close

endtime = Process.clock_gettime(Process::CLOCK_MONOTONIC)
elapsed = endtime - starttime
puts "Elapsed: #{elapsed} #{ARGV[1]}"
