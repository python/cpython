ipBadLapErr = 			-23000			# bad network configuration 
ipBadCnfgErr = 			-23001			# bad IP configuration error 
ipNoCnfgErr = 			-23002			# missing IP or LAP configuration error 
ipLoadErr = 			-23003			# error in MacTCP load 
ipBadAddr = 			-23004			# error in getting address 
connectionClosing = 	-23005			# connection is closing 
invalidLength = 		-23006
connectionExists = 		-23007			# request conflicts with existing connection 
connectionDoesntExist =	-23008			# connection does not exist 
insufficientResources =	-23009			# insufficient resources to perform request 
invalidStreamPtr = 		-23010
streamAlreadyOpen = 	-23011
connectionTerminated = 	-23012
invalidBufPtr = 		-23013
invalidRDS = 			-23014
invalidWDS = 			-23014
openFailed = 			-23015
commandTimeout = 		-23016
duplicateSocket = 		-23017

# Error codes from internal IP functions 
ipDontFragErr = 		-23032			# Packet too large to send w/o fragmenting 
ipDestDeadErr = 		-23033			# destination not responding
icmpEchoTimeoutErr = 	-23035			# ICMP echo timed-out 
ipNoFragMemErr = 		-23036			# no memory to send fragmented pkt 
ipRouteErr = 			-23037			# can't route packet off-net 

nameSyntaxErr =  		-23041		
cacheFault = 			-23042
noResultProc = 			-23043
noNameServer = 			-23044
authNameErr = 			-23045
noAnsErr = 				-23046
dnrErr = 				-23047
outOfMemory = 			-23048
