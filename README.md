# Fluent Bit Dashboard

The Fluent Bit Dashboard exposes a web interface to monitor the internals of Fluent Bit.

## Build and Run

The web service and is built on top of [Duda I/O](http://duda.io) framework, so in order to build it we need to get the stack and prepare the environment.

### Get Duda Client Manager and Duda stack

Duda Client Manager or _DudaC_ for short, it's a helper tool to gather the stack and it dependencies, steps:

```shell
$ git clone https://github.com/monkey/dudac
```

Now get into the _dudac_ directory and initialize the environment:

```shell
$ cd dudac
$ ./dudac -g
```

A successful build should looks like this:

```shell
$ ./dudac -g
Duda Client Manager - v0.31
http://duda.io
http://monkey-project.com

[i] HOME        : /home/edsiper/.dudac/
[i] STAGE       : /home/edsiper/.dudac/stage/
[+] Monkey: cloning source code                                            [OK]
[+] Duda: cloning source code                                              [OK]
[+] GIT Monkey  : switch HEAD to 'dst-1'                                   [OK]
[+] GIT Duda    : switch HEAD to 'dst-1'                                   [OK]
[+] GIT Monkey  : archive                                                  [OK]
[+] GIT Duda    : archive                                                  [OK]
[+] Monkey      : prepare build                                            [OK]
[+] Monkey      : building                                                 [OK]
```

### Build and run the Dashboard

Get this Dashboard service:

```shell
$ git clone https://github.com/fluentbit-dashboard
```

now join the dashboard path and launch the service with _dudac_:

```shell
$ cd fluentbit-dashboard/service
$ /path/to/dudac/./dudac -w .
```

if everything went fine the output should be similar to this:

```
$ ~/dudac/./dudac -w .
Duda Client Manager - v0.31
http://duda.io
http://monkey-project.com

[i] HOME        : /home/edsiper/.dudac/
[i] STAGE       : /home/edsiper/.dudac/stage/
[+] WebService  : clean                                                    [OK]
[+] WebService  : build                                                    [OK]
[+] Monkey      : configure HTTP Server                                    [OK]
[+] Service Up  : http://localhost:2001/fluentbit/
```

Now you can access the Dashboard through the [http://localhost:2001/fluentbit](http://localhost:2001/fluentbit) link.

## Contact

Eduardo Silva <eduardo@treasure-data.com>
