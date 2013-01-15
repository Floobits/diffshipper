diffshipper(1) -- Floobits diff shipper. The mediocre fall-back solution for people with unsupported editors.
=============================================

## SYNOPSIS

`diffshipper -r ROOM_NAME -o ROOM_OWNER -u USERNAME -s API_SECRET PATH`


## DESCRIPTION

Join a Floobits room and monitor files in PATH for changes.


## OPTIONS

  * `--create-room`       Create a room and add PATH
  * `-D`                  Enable debug output
  * `-h HOST`             Host
  * `-o OWNER`            Room owner
  * `-p PORT`             Port
  * `-r ROOMNAME`         Room to join
  * `--room-perms PERM`   Used with `--create-room`. 0 is private, 1 is readable by anyone, 2 is writeable by anyone
  * `-s SECRET`           API secret
  * `-u USERNAME`         Username
  * `-v`                  Print version and exit


## EXAMPLES

`diffshipper -r testroom -o myuser -u myuser -s ve3aiCoo ~/code/collab_project/`

Join https://floobits.com/r/myuser/testroom/ and keep files in ~/code/collab_project/ synced with that room.
