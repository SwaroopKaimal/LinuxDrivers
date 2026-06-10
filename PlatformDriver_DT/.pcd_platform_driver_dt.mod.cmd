savedcmd_pcd_platform_driver_dt.mod := printf '%s\n'   pcd_platform_driver_dt.o | awk '!x[$$0]++ { print("./"$$0) }' > pcd_platform_driver_dt.mod
