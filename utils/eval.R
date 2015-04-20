library(plyr)


while(T) {

dat <- read.csv("/tmp/csma.csv", sep="\t", header=F)
head(dat)

sdr  <- subset(dat, V3=="01:02:03:04:05:06")
alix <- subset(dat, V3=="12:34:56:78:90:ab")

time_steps <- 0:ceiling(max(dat$V1))

sdr_packets <- table(cut(sdr$V1, time_steps))
alix_packets <- table(cut(alix$V1, time_steps))

plot(time_steps, c(0, sdr_packets), col='blue', type='l', ylim=c(0, max(c(sdr_packets, alix_packets))))
lines(time_steps, c(0, alix_packets)+3, col='red')

Sys.sleep(1)
}

