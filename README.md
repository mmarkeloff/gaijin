# Tests

```bash
> docker build -t cache-test -f Dockerfile.test . && docker run cache-test
```

# Service

```bash
> docker build -t cache-service -f Dockerfile.service . && docker run --net=host --name cache-service-instance cache-service
```

```bash
[1749651384335494380,"info",["flushing cache",["to","config.txt"],["size",9]]]
[1749651385322142958,"info",["new connection accepted",["cid",5]]]
[1749651389337511342,"info",["stat",["read requests",9884],["write requests",116],["processed in last 5 seconds",0]]]
[1749651394340582632,"info",["stat",["read requests",9884],["write requests",116],["processed in last 5 seconds",0]]]
[1749651399344423159,"info",["stat",["read requests",13539],["write requests",153],["processed in last 5 seconds",3692]]]
[1749651404348384606,"info",["stat",["read requests",14826],["write requests",174],["processed in last 5 seconds",1308]]]
[1749651405763204331,"info",["connection was closed",["id",5]]]
```

# Client

```bash
> docker build -t cache-client -f Dockerfile.client . && docker run --net=host --name cache-client-instance cache-client
```

```bash
[1749651400755035369,"info",["received",["data","qat=719 (reads=1609 writes=12)"]]]
[1749651400756127116,"info",["received",["data","mat=506 (reads=1698 writes=25)"]]]
[1749651400757212409,"info",["received",["data","bat=812 (reads=1630 writes=25)"]]]
[1749651400758312101,"info",["received",["data","tat=492 (reads=1681 writes=14)"]]]
[1749651400759399614,"info",["received",["data","mat=506 (reads=1699 writes=25)"]]]
[1749651400760488207,"info",["received",["data","rat=808 (reads=1661 writes=19)"]]]
[1749651400761584869,"info",["received",["data","oao=926 (reads=1632 writes=19)"]]]
[1749651400802144501,"info",["received",["data","bat=812 (reads=1631 writes=25)"]]]
```
