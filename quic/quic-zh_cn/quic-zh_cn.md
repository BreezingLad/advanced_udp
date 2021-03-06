
什么是QUIC
QUIC（Quick UDP Internet Connections）是谷歌针对互联网定制的一种全新的传输层协议。
使用QUIC协议，开发者只需要很少、甚至不需要额外的代码，就能解决现代WEB应用程序的一系列传输层和应用层问题。QUIC 基于UDP协议，基本实现了TCP+TLS+HTTP2的功能。现有的协议往往受限与已有的客户端、代理等，独立的QUIC安装协议栈即可使用。

对比TCP+TLS+HTTP2模式，QUIC主要优点有：
低延迟的连接建立
更好的拥塞控制
使用多路技术避免线头阻塞（HOL）
前向纠错
连接迁移

连接建立
完整的连接建立的描述，参见QUIC设计文档[1]。
简单点说，大多数时候，QUIC在发送数据之前，不会先耗费往返时间进行握手，而TCP+TLS需要1-3个往返时间。
在客户端第一次发起连接到服务端时，客户端会花费一个往返时间来保证握手信息的完整性。客户端发送一个早期的client hello（CHLO）报文——空数据报文，服务端会回复一个rejection(REJ)——该报文包含客户端需要进行下去的信息，比如，源地址标识、服务端证书。此时，客户端就可以发送正常的CHLO。当然，如果本地缓存了服务端的证书——已连接过，客户端会直接使用之前连接的证书发起加密的连接到服务端。

拥塞控制
QUIC 支持可插入式的拥塞控制，并且，相比TCP，它为拥塞控制算法提供了更丰富的信息。目前，谷歌实现的QUIC的拥塞算法，是TCP中的Cubic的重实现版，它是谷歌尝试的替代方案。
比如，QUIC的发送报文和重传报文使用不同的序列号，QUIC的发送端可以区分发送ACK和重传ACK ——避免了TCP重传报文的二义性问题[1]。QUIC ACK 携带包处理延迟——对端收到一个包到ack发出的时间、线性增长的序列号。这些都能使往返时间计算更精确。
QUIC ack 帧支持超过256个NACK，所以相比TCP的SACK，QUIC 在重排时更具有弹性，并且在重排或丢失时，可以保持更多的数据在线路中。关于对端到底已收到哪些数据，在客户端和服务器中有更精确的描述。

多路技术
基于TCP上的HTTP2最大的问题之一就是线头阻塞。应用层将TCP连接视为字节流。当一个TCP报文丢失——甚至该报文已经到达了，只是还在队列中，该HTTP2上的所有流到不会继续，直到该丢失报文被遥远的对端重传、接收。
由于QUIC是完全为了多路技术而设计的，某条流上丢失报文的数据通常只会影响该条流。其它流上的帧仍然可以传送，没有数据丢失的流也可以被组装然后交付给应用层。

前向纠错
为了在不重传就能覆盖丢失的报文，QUIC可以通过一个FEC报文补足一组报文。就像RAID-4，FEC报文是该FEC组中的奇偶校验包[2]。如果该组中有一个包丢失了，该丢失的报文可以通过该组的FEC包和该组中的其它数据包恢复。发送方可以决定是否发送FEC报文来优化特定的场景（比如请求的开始和结束）

连接迁移
QUIC连接由客户端随机产生一个64位的ID号来标识。相应的，TCP 连接是由4元组来标识：源IP、源端口、目的IP和目的端口。这就意味着如果一个客户端改变了IP地址（比如从wifi切换到移动数据）或者端口（NAT 转换表丢失，重新进行端口映射），任何已有的TCP连接都失效了。当QUIC客户端改变IP地址时，它只要在新的IP上继续使用之前的连接ID，即使是在发送的请求，也不会被中断。

[1]QUIC 设计文档：https://docs.google.com/document/d/1g5nIXAIkN_Y-7XJW5K45IblHd_L2f5LTaDUDwvZ5L6g/edit
[2]TCP中重传报文中计算的往返时间是不正确的，通常是忽略该次计算。
[3]FEC报文就像是RAID-4中的奇偶校验盘，普通的报文就像RAID-4中的普通数据盘。
