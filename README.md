APFSPayloadFixup
===================

An open source kernel extension providing patches for kernel and com.apple.filesystems.apfs.

#### Features
- This plugin resolves sporadical boot panic "The global payload bytes pointer is NULL" by waiting for
available payload in overrided kernel method vfs_mountroot().

#### Boot-args
- `-apfspayloadfixupoff` disables kext loading
- `-apfspayloadfixupdbg` turns on debugging output
- `-apfspayloadfixupbeta` enables loading on unsupported osx

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu)
- [Sherlocks](http://www.insanelymac.com/forum/user/980913-sherlocks/) for the idea & testing
- [dokterdok](https://github.com/dokterdok/Continuity-Activation-Tool/) for patch
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for writing the software and maintaining it
