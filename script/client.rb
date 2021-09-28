require 'socket'

s = UNIXSocket.new("/home/francesco/Documenti/Unipi/Sistemi Operativi/Progetto/filestorageserver/socket/l.sock")

s.write("./testfiles/#{ARGV[0]}.txt\n")


s.each_line do |line|
	puts line
end
s.close
