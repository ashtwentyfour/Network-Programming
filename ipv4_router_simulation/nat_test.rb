class PDU
	def self.parse(string)
		pattern = /
			(\d+\.\d+\.\d+\.\d+):(\d+)->	# source IP
			(\d+\.\d+\.\d+\.\d+):(\d+).*	# destination IP
			\(([A-Za-z0-9]+)(-.*)?\)	# egress interface
			.*ttl\s(\d+)			# TTL
		/x

		if string =~ pattern
			ret = self.new

			ret.source_ip = $1
			ret.source_port = $2
			ret.destination_ip = $3
			ret.destination_port = $4
			ret.interface = $5
			ret.ttl = $7
			ret
		else
			nil
		end
	end

	def response_string
		"#{@interface}\t" +
		"#{@destination_ip}\t" +
		"#{@source_ip}\t" +
		"6\t" +
		"64\t" +
		"#{@destination_port}\t" +
		"#{@source_port}"
	end

	attr_accessor :source_ip, :source_port
	attr_accessor :destination_ip, :destination_port
	attr_accessor :interface, :ttl
end

# read the NAT rules so we know which IPs to send responses to
source_ips = []
File.open("nat.txt", "r") do |nat_rules|
	nat_rules.each_line do |nat_rule|
		nat_rule =~ /[A-Za-z0-9]+\s+(\d+\.\d+\.\d+\.\d+)/
		source_ips << $1
	end
end

# open child process for the program under test
IO.popen(ARGV[0], "r+") do |child|
	line_count = 0

	# read the PDUs from pdus.txt and write them to the child process
	File.open("pdus.txt", "r") do |pdus|
		pdus.each_line do |line|
			child.puts line
			child.flush
			line_count += 1
		end
	end

	child.flush

	STDERR.puts "[nat_test] expecting #{line_count} PDUs"

	# read PDUs output by the child process
	line_count.times do
		line = child.readline
		puts line
		STDOUT.flush

		incoming_pdu = PDU.parse(line)
		if incoming_pdu
			# for any PDUs coming from the NATted IPs, 
			# reverse source and destination and feed back to child process

			if source_ips.include?(incoming_pdu.source_ip)
				STDERR.puts "[nat_test] generating response"
                                #child.puts "[nat_test] #{incoming_pdu.response_string.inspect}"
				child.puts incoming_pdu.response_string
				child.flush
			end
			STDERR.puts "[nat_test] #{incoming_pdu.inspect}"
		elsif line =~ /discarded/
			STDERR.puts "[nat_test] ignoring discarded PDU"
		else
			STDERR.puts "[nat_test] couldn't parse previous line"
		end
	end

	child.close_write

	# simply echo the remaining PDUs from the child
	child.each_line do |line|
		puts line
	end

end
