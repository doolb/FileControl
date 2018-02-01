ref
===

## modify file size
* refresh dir / file details
  * `IRP_MJ_DIRECTORY_CONTROL`
  * `IRP_MN_QUERY_DIRECTORY`
  * `FileIdBothDirectoryInformation`
* file general
  * `FileBothDirectoryInformation (0n3)`

## libcrc
https://github.com/lammertb/libcrc.git

windbg fitler
=============
#Include:
[Mini Filter]

#Exclude:
    DR;HarddiskVolume1


    [Mini Filter]@volumeDetech: setup volume context: Name="D:",Device:08.00000000, Guid:\??\Volume{e6819827-fd02-11e7-8763-000c29dc5a92}, SectSize=0x0200, \Device\HarddiskVolume3
