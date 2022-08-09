Magisk Detector
==============================

反Magisk Hide
-------------
Magisk Hide的理论核心是挂载命名空间：magiskd等待zygote子进程的挂载命名空间与父进程分离后，对子进程卸载所有Magisk挂载的内容。
由于挂载命名空间的特性，卸载操作会影响这个子进程的子进程，但不影响zygote。
zygote是所有应用程序进程的父进程，如果zygote被Magisk Hide处理，所有应用都丢失root。
zygote启动新进程时有一个`MountMode`参数，当它是`Zygote.MOUNT_EXTERNAL_NONE`时，新进程不挂载存储空间，也就没有挂载命名空间分离步骤。

符合的情况有两种，一个是应用appops的读取存储空间op为忽略，一个是该进程为隔离进程。前者应用本身无法操作，后者自Android4.1开始支持。
另外，隔离进程还有一个有趣的特性：随机UID，它每次运行时的UID都不一样。
这个有趣的特性导致了在检测挂载命名空间之前，Magisk Hide就跳过这个进程，不处理它。

新版本的Android系统强制全部zygote子进程分离挂载命名空间，并且新版本的Magisk单独检测隔离进程，支持其随机UID特性。因此旧方法作废。

Magisk Hide的实现核心是ptrace：magiskd跟踪zygote进程，监控fork和clone操作，即关注子进程创建及其线程创建。
被触发后读取/proc/pid/cmdline和uid判断是否为目标进程。 一般应用进程的cmdline在Java设置，此时已有主线程及JVM虚拟机工作线程。
在加载用户代码前，会至少有一次binder调用，使binder线程启动，此操作触发magiskhide卸载挂载。
唯一的例外是app zygote，它和zygote一样通过socket通信，没有binder线程。在设置cmdline和加载用户代码之间没有线程启动，
因此，可以检测是否被ptrace来判断magiskhide的存在，即使不在隐藏列表中，只要开启功能，就会被发现。

检测Magisk模块
-------------
Magisk模块虽然能在文件系统上隐藏，但修改内容已经载入进程内存，检查进程的maps就能发现。
maps显示的数据包含载入文件所在的设备。Magisk模块会导致某些文件的路径在system或vendor等分区，但显示的设备位置却是data分区。

检测MagiskSU
------------
正常情况下，应用不能连接不是自己建立的socket，但Magisk修改了SELinux。所有应用都能连接magisk域的socket。
每个Magisk的su进程都会建立一个socket，尝试连接所有socket，没有被SELinux拒绝的socket数量，就是su进程的数量。
此检测方法可靠程度完全取决于SELinux规则的严格程度，Android版本太低没有强制执行SELinux，大于等于Android10系统不允许列出socket。
因此删除。

测试SELinux政策
--------------
SU软件在修改SELinux政策时一定要注意以下原则，否则会留下安全漏洞和被检测的途径。
- 来源域和目标域，有一方是su程序自定义域，添加规则是安全的；
- 双方都是非应用程序域的，需要有合理且必要的原因；
- 有一方是应用域，添加规则非常危险。如果是来源域，应该拒绝添加。

Magisk已经修复，因此删除。

检测init.rc修改
--------------
随机只有在无法遍历的情况下才有效。如果可以遍历，使用统计方法即可准确找出每次都不一样的东西。
简单来说：getprop，列出`init.svc.*`，会有几个服务的名字每次开机都不一样。很无聊的检测，因此删除。
