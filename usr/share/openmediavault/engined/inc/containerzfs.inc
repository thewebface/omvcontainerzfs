<?php
require_once("openmediavault/system.inc");

class OMVFilesystemBackendContainerZFS extends OMVFilesystemBackendAbstract {
	public function __construct() {
		parent::__construct();
		$this->type = "containerzfs";
		$this->properties = self::PROP_POSIX_ACL;
	}
	public function getImpl($args) {
		return new OMVFilesystemContainerZFS($args);
	}
	public function isTypeOf($fsName) {
		$fsList = $this->enumerate();
		return isset($list[$fsName]);
	}
	public function enumerate() {
		$cmd = "mount -t zfs";
		$output = [];
		@OMVUtil::exec($cmd, $output);

		$result = [];

		foreach ($output as $line) {
			if (! preg_match('#^([^ ]+) on (/.*) type zfs \((.*)\)$#', $line, $matches))
				continue;

			list(, $deviceFile, $mountPoint, $attrs) = $matches;

			$result[$deviceFile] = [
				"devicefile" => $deviceFile,
				"uuid" => "",
				"label" => $deviceFile,
				"type" => "containerzfs",
				"mountPoint" => $mountPoint,
				"attrs" => $attrs,
			];
		}

		$this->syncConfig($result);

		return $result;
	}
	public function syncConfig($devices=null) {
		global $xmlConfig;

		if (is_null($devices)) {
			$devices = $this->enumerate();
		}

		// Add new entries
		foreach ($devices as $fsName=>$fsInfo) {
			$xpath = "//system/fstab/mntent[fsname='{$fsName}' and dir='{$fsInfo['mountPoint']}' and type='containerzfs']";
			if (!($xmlConfig->exists($xpath))) {
				$uuid = OMVUtil::uuid();
				$object = array(
						"uuid" => $uuid,
						"fsname" => $fsName,
						"dir" => $fsInfo['mountPoint'],
						"type" => $fsInfo['type'],
						"opts" => $fsInfo['attrs'],
						"freq" => "0",
						"passno" => "0",
						"hidden" => "0",
					       );
				$xmlConfig->set("//system/fstab",array("mntent" => $object));
			}
		}

		// Remove old entries
		$xpath = "//system/fstab/mntent[type='containerzfs']";
		$mntents = $xmlConfig->getList($xpath);

		if (is_null($mntents)) {
			throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
		}

		foreach ($mntents as $mntentk => $mntentv) {
			if (!isset($devices[$mntentv['fsname']]) || $devices[$mntentv['fsname']]['mountPoint'] != $mntentv['dir']) {
				$xpath = "//system/fstab/mntent[fsname='{$mntentv['fsname']}' and dir='{$mntentv['dir']}' and type='containerzfs']";
				$xmlConfig->delete($xpath);
			}
		}
	}
	public function isBlkidEnumerated() {
		return FALSE;
	}
}

class OMVFilesystemContainerZFS extends OMVFilesystemAbstract {
	public function __construct($fsName) {
		$this->deviceFile = $fsName;
		$this->type = "containerzfs";
		$this->usage = "filesystem";

		$this->dataCached = FALSE;
	}
	/**
	 * Get the filesystem detailed information.
	 * @protected
	 * @return TRUE if successful, otherwise FALSE.
	 */
	protected function getData() {
		if ($this->dataCached !== FALSE)
			return TRUE;

		$cmd = "mount -t zfs";
                $output = [];
                @OMVUtil::exec($cmd, $output);

		foreach ($output as $line) {
			if (! preg_match('#^([^ ]+) on (/.*) type zfs \((.*)\)$#', $line, $matches))
				continue;

			list(, $deviceFile, $mountPoint, $attrs) = $matches;

			if ($deviceFile == $this->deviceFile) {
				$this->label = $deviceFile;
				$this->mountPoint = $mountPoint;
				$this->attrs = $attrs;

				$this->dataCached = TRUE;
				return TRUE;
			}
		}

		return FALSE;
	}
	/**
	 * Refresh the cached information.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function refresh() {
		$this->dataCached = FALSE;
		if ($this->getData() === FALSE)
			return FALSE;
		return TRUE;
	}
	/**
	 * Checks if the filesystem exists.
	 * @return TRUE if the device exists, otherwise FALSE.
	 */
	public function exists() {
		if ($this->getData() === FALSE)
			return FALSE;
		return TRUE;
	}
	/**
	 * Check if the filesystem has an UUID.
	 * @return FALSE.
	 */
	public function hasUuid() {
		return FALSE;
	}
	/**
	 * Get the UUID of the filesystem.
	 * @return FALSE.
	 */
	public function getUuid() {
		return FALSE;
	}
	/**
	 * Check if the filesystem has a label.
	 * @return Returns TRUE
	 */
	public function hasLabel() {
		if (FALSE === ($label = $this->getLabel()))
			return FALSE;
		return !empty($label);
	}
	/**
	 * Get the filesystem label.
	 * @return The filesystem label, otherwise FALSE.
	 */
	public function getLabel() {
		if ($this->getData() === FALSE)
			return FALSE;
		return $this->label;
	}
	/**
	 * Get the filesystem type, e.g. 'ext3' or 'vfat'.
	 * @return The filesystem type, otherwise FALSE.
	 */
	public function getType() {
		if ($this->getData() === FALSE)
			return FALSE;
		return $this->type;
	}
	/**
	 * Get the partition scheme, e.g. 'gpt', 'mbr', 'apm' or 'dos'.
	 * @deprecated
	 * @return The filesystem type, otherwise FALSE.
	 */
	public function getPartitionType() {
		return $this->getPartitionScheme();
	}
	/**
	 * Get the partition scheme, e.g. 'gpt', 'mbr', 'apm' or 'dos'.
	 * @return The filesystem type, otherwise FALSE.
	 */
	public function getPartitionScheme() {
		return FALSE;
	}
	/**
	 * Get the usage, e.g. 'other' or 'filesystem'.
	 * @return The filesystem type, otherwise FALSE.
	 */
	public function getUsage() {
		if ($this->getData() === FALSE)
			return FALSE;
		return $this->usage;
	}
	/**
	 * Get the partition entry information.
	 * @return An array with the fields \em scheme, \em uuid, \em type,
	 *   \em flags, \em number, \em offset, \em size and \em disk,
	 *   otherwise FALSE.
	 */
	public function getPartitionEntry() {
		return FALSE;
	}
	/**
	 * Get the device path of the filesystem, e.g /dev/sdb1.
	 * @return The device name, otherwise FALSE.
	 */
	public function getDeviceFile() {
		if ($this->getData() === FALSE)
			return FALSE;
		return $this->deviceFile;
	}
	/**
	 * Get the canonical device file, e.g. /dev/root -> /dev/sde1
	 */
	public function getCanonicalDeviceFile() {
		return $this->deviceFile;
	}
	/**
	 * Get the device path by UUID, e.g.
	 * \li /dev/disk/by-uuid/ad3ee177-777c-4ad3-8353-9562f85c0895
	 * \li /dev/disk/by-uuid/2ED43920D438EC29 (NTFS)
	 * @return The device path (/dev/disk/by-uuid/xxx) if available,
	 *   otherwise /dev/xxx will be returned. In case of an error FALSE
	 *   will be returned.
	 */
	public function getDeviceFileByUuid() {
		return FALSE;
	}
	/**
	 * Get the device file of the storage device containing this
	 * file system. Example:
	 * <ul>
	 * \li /dev/sdb1 => /dev/sdb
	 * \li /dev/cciss/c0d0p2 => /dev/cciss/c0d0
	 * </ul>
	 * @return The device file of the underlying storage device,
	 *   otherwise FALSE.
	 */
	public function getStorageDeviceFile() {
		return FALSE;
	}
	/**
	 * Get the filesystem block size.
	 * @return The block size, otherwise FALSE.
	 */
	public function getBlockSize() {
		return FALSE;
	}
	/**
	 * Get the mount point of the given filesystem.
	 * @return The mountpoint of the filesystem or FALSE.
	 */
	public function getMountPoint() {
		if ($this->getData() === FALSE)
			return FALSE;
		return $this->mountPoint;
	}
	/**
	 * Get statistics from a mounted filesystem.
	 * @return The filesystem statistics if successful, otherwise FALSE. The
	 *   following fields are included: \em devicefile, \em type, \em blocks,
	 *   \em size, \em used, \em available, \em percentage and \em mountpoint.
	 *   Please note, the fields \em size, \em used and \em available are
	 *   strings and their unit is 'B' (bytes).
	 */
	public function getStatistics() {
		if ($this->getData() === FALSE)
			return FALSE;
		// Get the mount point of the filesystem.
		if (FALSE === ($mountPoint = $this->getMountPoint()))
			return FALSE;
		$cmd = sprintf('df -PTtzfs %s', escapeshellarg($mountPoint));
		$output = [];
		@OMVUtil::exec($cmd, $output, $result);
		if ($result !== 0 || count($output) != 2) {
			$this->setLastError($output);
			return FALSE;
		}
		$matches = preg_split("/[\s,]+/", $output[1]);
		if (0 !== strcasecmp($mountPoint, $matches[6]))
			continue;
		$result = array(
				"devicefile" => $this->deviceFile,
				"type" => $matches[1],
				"blocks" => $matches[2],
				"size" => bcmul($matches[2], "1024", 0),
				"used" => binary_convert($matches[3], "KiB", "B"),
				"available" => binary_convert($matches[4], "KiB", "B"),
				"percentage" => intval(trim($matches[5], "%")),
				"mountpoint" => $matches[6]
			       );
		return $result;
	}
	/**
	 * Check if a filesystem is mounted.
	 * @return TRUE if the filesystem is mounted, otherwise FALSE.
	 */
	public function isMounted() {
		if ($this->getData() === FALSE)
			return FALSE;
		return TRUE;
	}
	/**
	 * Mount the filesystem by its device file or UUID.
	 * @param options Additional mount options. Defaults to "".
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function mount($options = "") {
		return TRUE;
	}
	/**
	 * Unmount the filesystem.
	 * @param force Set to TRUE to force unmount. Defaults to FALSE.
	 * @param lazy Set to TRUE to lazy unmount. Defaults to FALSE.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function umount($force = FALSE, $lazy = FALSE) {
		return FALSE;
	}
	/**
	 * See if a directory is a mountpoint.
	 * @param
	 * @return TRUE if the directory is a mountpoint, otherwise FALSE.
	 */
	public static function isMountPoint($dir) {
		if (!is_dir($dir))
			return FALSE;
		if (FALSE === ($stat = stat($dir)))
			return FALSE;
		if (FALSE === ($stat2 = stat(sprintf("%s/..", $dir))))
			return FALSE;
		return (($stat.dev != $stat2.dev) || (($stat.dev == $stat2.dev) &&
					($stat.ino == $stat2.ino))) ? TRUE : FALSE;
	}
	/**
	 * Get the directory where the filesystem should be mounted to. Note,
	 * this path is OMV specific: /media/<FSUUID>.
	 * @param uuid The UUID of the filesystem.
	 * @return The path where to mount the filesystem, e.g.
	 *   /media/85732966-949a-4d8b-87d7-d7e6681f787e.
	 */
	public static function buildMountPath($uuid) {
		return sprintf("%s/%s", $GLOBALS['OMV_MOUNT_DIR'], $uuid);
	}
	/**
	 * Check if the given device file contains a file system.
	 * @param deviceFile The devicefile to check.
	 * @return TRUE if the devicefile has a file system, otherwise FALSE.
	 */
	public static function hasFileSystem($deviceFile) {
		return TRUE;
	}
}

OMVFilesystems::registerBackend(new OMVFilesystemBackendContainerZFS());

