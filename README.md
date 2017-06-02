# bb84_crypto
Qt5-based realisation of BB4

#Applications
Project is based on client-server topology.

Clients are Alice, Bob and Eve. Alice send message or key to Bob via quantum photon channel.
Eve is trying to spy sending data while Bob is trying to detect spying process.

Server is a Server application which is managing with Alice message. It applies Gauss noise, Poisson distribution or linear photon loss to the channel.
