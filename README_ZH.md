Magisk Detector
==============================

反Magisk Hide
-------------
Magisk Hide的核心是挂载命名空间，magiskd等待zygote子进程的挂载命名空间与父进程分离后，对子进程卸载所有Magisk挂载的内容。由于挂载命名空间的特性，卸载操作会影响这个子进程的子进程，但不会影响zygote。zygote是所有应用程序进程的父进程，如果zygote被Magisk Hide处理，所有应用都会丢失root。
zygote启动新进程时有一个`MountMode`参数，当它是`Zygote.MOUNT_EXTERNAL_NONE`时，新进程不挂载存储空间，也就没有挂载命名空间分离步骤。

符合的情况有两种，一个是应用appops的读取存储空间op为忽略，一个是该进程为隔离进程。前者应用本身无法操作，后者自Android4.1开始支持。
另外，隔离进程还有一个有趣的特性，就是随机UID，它每次运行时的UID都不一样。这个有趣的特性导致了在检测挂载命名空间之前，Magisk Hide就跳过了这个进程，不会处理它。

检测Magisk模块
-------------
Magisk模块虽然能在文件系统上隐藏，但载入进程内存的还是修改版文件，检查进程的maps就能发现。内核会标记载入文件的原始位置，于是Magisk模块会导致某些路径在system分区或vendor分区的文件，实际位置是data分区。

检测MagiskSU
------------
正常情况下，应用不能连接不是自己建立的socket，但Magisk修改了SELinux。所有应用都能连接magisk域的socket。每个Magisk的su进程都会建立一个socket，尝试连接所有socket，没有被SELinux拒绝的socket数量，就是su进程的数量。
