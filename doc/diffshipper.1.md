diffshipper(1) -- Floobits diff shipper. The mediocre fall-back solution for people with unsupported editors.
=============================================

## SYNOPSIS

`diffshipper -r ROOM_NAME -o ROOM_OWNER -u USERNAME -s API_SECRET PATH`


## DESCRIPTION

Join a Floobits room and monitor files in PATH for changes.


## OPTIONS

  * `-D`                  Enable debug output\n\
  * `-h HOST`             Host\n\
  * `-o OWNER`            Room owner\n\
  * `-p PORT`             Port\n\
  * `-r ROOMNAME`         Room to join\n\
  * `-s SECRET`           API secret\n\
  * `-u USERNAME`         Username\n\
  * `-v`                  Print version and exit\n\


## EXAMPLES

`diffshipper -r testroom -o myuser -u myuser -s ve3aiCoo ~/code/collab_project/`

Join https://floobits.com/r/myuser/testroom/ and keep files in ~/code/collab_project/ synced with that room.
