# dogcom [![travis-ci](https://travis-ci.org/mchome/dogcom.svg "Build status")](https://travis-ci.org/mchome/dogcom) [![badge](https://img.shields.io/badge/%20built%20with-%20%E2%9D%A4-ff69b4.svg "build with love")](https://github.com/mchome/dogcom) [![version](https://img.shields.io/badge/stable%20-%20v1.6.2-4dc71f.svg "stable version")](https://github.com/mchome/dogcom/tree/v1.6.2)

[Drcom-generic](https://github.com/drcoms/drcom-generic) implementation in C.

```
Usage:
        dogcom -m <dhcp/pppoe> -c <FILEPATH> [options <argument>]...

Options:
        --mode <dhcp/pppoe>, -m <dhcp/pppoe>  set your dogcom mode
        --conf <FILEPATH>, -c <FILEPATH>      import configuration file
        --bindip <IPADDR>, -b <IPADDR>        bind your ip address(default is 0.0.0.0)
        --log <LOGPATH>, -l <LOGPATH>         specify log file
        --802.1x, -x                          enable 802.1x
        --daemon, -d                          set daemon flag
        --eternal, -e                         set eternal flag
        --verbose, -v                         set verbose flag
        --help, -h                            display this help
```

Config file is compatible with [drcom-generic](https://github.com/drcoms/drcom-generic).

#### Example:

```bash
$ dogcom -m dhcp -c dogcom.conf
$ dogcom -m dhcp -c dogcom.conf -l /tmp/dogcom.log -v
$ dogcom -m dhcp -c dogcom.conf -d # (PS: only on Linux build)
$ dogcom -m pppoe -c dogcom.conf -x # (PS: only on Linux build)
$ dogcom -m pppoe -c dogcom.conf -e # eternal dogcoming (default times is 5)
$ dogcom -m pppoe -c dogcom.conf -v
$ dogcom -m dhcp -c dogcom.conf -b 10.2.3.12 -v
```

#### To build:

```bash
$ make # Linux
$ make win32=y # Windows(MinGW)
$ make test=y # For testing purposes
$ make force_encrypt=y # Force open encrypt mode in PPPoE version
```

#### Openwrt-package
[https://github.com/mchome/openwrt-dogcom](https://github.com/mchome/openwrt-dogcom)

#### Tutorial
[![asciicast](https://asciinema.org/a/9j7cj1s61jiczx2s0206tosjr.png)](https://asciinema.org/a/9j7cj1s61jiczx2s0206tosjr)

### Thanks:
- [gdut-drcom](https://github.com/chenhaowen01/gdut-drcom 'chenhaowen01')
- [jlu-drcom-client](https://github.com/drcoms/jlu-drcom-client/tree/master/C-version 'feix')
- [leetking](https://github.com/leetking 'leetking')

### Special thanks:
- [Drcom-generic](https://github.com/drcoms/drcom-generic 'ly0')

### License:
![AGPL V3](https://cloud.githubusercontent.com/assets/7392658/20011165/a0caabdc-a2e5-11e6-974c-8d4961c7d6d3.png)
