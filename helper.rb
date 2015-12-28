module Py2rbHelper
	def print(list)
		if(list.class == Array)
			Kernel.print "["
			list.first(list.size - 1).each { |x| 
				modprintforstring x
				Kernel.print ", "
			}
			modprintforstring list.last
			Kernel.print "]"
		else
			Kernel.print "#{list}"
		end
	end
	def puts(list)
		print(list)
		Kernel.print "\n";
	end
	def included
		puts "included"
	end
	private
	def modprintforstring x
		if x.class == String
			print "'#{x}'" 
		else
			print x
		end
	end
end
include Py2rbHelper
