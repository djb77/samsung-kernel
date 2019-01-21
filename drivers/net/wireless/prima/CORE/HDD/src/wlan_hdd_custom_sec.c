#include <linux/fcntl.h>
#include <linux/fs.h>

#include <vos_api.h>	// Qualcomm vOSS API

#define SEC_LOG						printk

////////////////////////////////////////////////////////////////////////////////
// Read MAC from efs by sh2011.lee@samsung 2011-12-15
#ifdef SEC_READ_MACADDR

#define SEC_MAC_FILEPATH	"/efs/wifi/.mac.info"
#define MAX_RETRY					5

v_MACADDR_t sec_mac_addrs[VOS_MAX_CONCURRENCY_PERSONA];
static int sec_mac_loaded;

static int wlan_hdd_read_mac_addr(unsigned char *mac);

#if 1
unsigned char *wlan_hdd_sec_get_mac_addr(int i)
{
	if (!sec_mac_loaded) {
		// station mac
		if (wlan_hdd_read_mac_addr(sec_mac_addrs[0].bytes) < 0)
			return NULL;	// Cannot read MAC from efs

		// hotspot mac == sta mac
		memcpy(&sec_mac_addrs[1].bytes[0], &sec_mac_addrs[0].bytes[0], 6);
		sec_mac_addrs[1].bytes[0] |= 2;

		// p2p mac
		memcpy(&sec_mac_addrs[2].bytes[0], &sec_mac_addrs[0].bytes[0], 6);
		sec_mac_addrs[2].bytes[0] |= 4;

		// monitor mac == sta mac
		memcpy(&sec_mac_addrs[3].bytes[0], &sec_mac_addrs[0].bytes[0], 6);
		sec_mac_addrs[3].bytes[0] |= 8;

		sec_mac_loaded = 1;
	}

	return sec_mac_addrs[i].bytes;
}
#endif

static int wlan_hdd_read_mac_addr(unsigned char *mac)
{
	struct file *fp      = NULL;
	char macbuffer[18]   = {0};
	mm_segment_t oldfs   = {0};
	char randommac[3]    = {0};
	char buf[18]         = {0};
	char *filepath       = SEC_MAC_FILEPATH;
	int ret = 0;
	int i;
	int create_random_mac = 0;
	struct dentry *parent;
	struct dentry *dentry;
	struct inode *p_inode;
	struct inode *c_inode;

	for (i = 0; i < MAX_RETRY; ++i) {
		fp = filp_open(filepath, O_RDONLY, 0);
		if (IS_ERR(fp) || create_random_mac == 1) {
			/* File Doesn't Exist. Create and write mac addr.*/
			fp = filp_open(filepath, O_RDWR | O_CREAT, 0660);
			if (IS_ERR(fp)) {
				SEC_LOG("[WIFI] %s: cannot create a file\n", filepath);
				return -1;
			}
			oldfs = get_fs();
			set_fs(get_ds());
			/* set uid , gid of parent directory */
			dentry = fp->f_path.dentry;
			parent = dget_parent(dentry);
			c_inode = dentry->d_inode;
			p_inode = parent->d_inode;
			c_inode->i_uid = p_inode->i_uid;
			c_inode->i_gid = p_inode->i_gid;

			/* Generating the Random Bytes for 3 last octects of the MAC address */
			get_random_bytes(randommac, 3);

			sprintf(macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X\n",
					0x00, 0x12, 0x34, randommac[0], randommac[1], randommac[2]);
			//SEC_LOG("[WIFI] The randomly generated MAC ID: %s", macbuffer);

			if (fp->f_mode & FMODE_WRITE) {
				ret = fp->f_op->write(fp, (const char *)macbuffer, sizeof(macbuffer), &fp->f_pos);
				if (ret < 0)
					SEC_LOG("[WIFI] MAC write to %s is failed: %s", filepath, macbuffer);
				else
					SEC_LOG("[WIFI] MAC write to %s is success: %s", filepath, macbuffer);
			}
			set_fs(oldfs);
		}

		/* Reading the MAC Address from .mac.info file (the existed file or just created file)*/
		ret = kernel_read(fp, 0, buf, 17);
		buf[17] = '\0';   // to prevent abnormal string display when mac address is displayed on the screen.
		//SEC_LOG("[WIFI] Read MAC: [%s]\n", buf);

		if (ret != 17 || strncmp(buf, "00:00:00:00:00:00", 17) == 0) {
			filp_close(fp, NULL);
			create_random_mac = 1;
			continue;// Retry!
		}

		break;	// Read MAC is success
	}

	if (fp)
		filp_close(fp, NULL);

	if (ret) {
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned int *)&(mac[0]), (unsigned int *)&(mac[1]),
				(unsigned int *)&(mac[2]), (unsigned int *)&(mac[3]),
				(unsigned int *)&(mac[4]), (unsigned int *)&(mac[5]));
		return 0;	// success
	}

	SEC_LOG("[WIFI] Reading MAC from the '%s' is failed.\n", filepath);
	return -1;	// failed
}

#endif /* SEC_READ_MACADDR */

////////////////////////////////////////////////////////////////////////////////
// Control power save mode on/off by sh2011.lee@samsung 2012-01-10
#ifdef SEC_CONFIG_PSM

#define SEC_PSM_FILEPATH	"/data/misc/conn/.psm.info"

unsigned int wlan_hdd_sec_get_psm(unsigned int original_value)
{
	struct file *fp      = NULL;
	char *filepath       = SEC_PSM_FILEPATH;
	int i, value;

	for (i = 0; i < MAX_RETRY; ++i) {
		fp = filp_open(filepath, O_RDONLY, 0);
		if (!IS_ERR(fp)) {
			//kernel_read(fp, fp->f_pos, &value, 1);
			kernel_read(fp, 0, (char *)&value, 1);
			SEC_LOG("[WIFI] PSM: [%u]\n", value);
			if (value == '0')
				original_value = value - '0';
			break;
		}
	}
	if (fp && !IS_ERR(fp))
		filp_close(fp, NULL);

	return original_value;
}

#endif /* SEC_CONFIG_PSM */

////////////////////////////////////////////////////////////////////////////////
// write version info in /data/.fs
#ifdef SEC_WRITE_VERSION_IN_FILE
#include "qwlan_version.h"

#define SEC_VERSION_FILEPATH	"/data/misc/conn/.wifiver.info"

int wlan_hdd_sec_write_version_file(char *riva_version)
{
	int ret = 0;
	struct file *fp = NULL;
	char strbuffer[50]   = {0};
	mm_segment_t oldfs   = {0};

	oldfs = get_fs();
	set_fs(get_ds());

	fp = filp_open(SEC_VERSION_FILEPATH, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (IS_ERR(fp))
		SEC_LOG("%s: can't create file : %s", __func__, SEC_VERSION_FILEPATH);
	else {
		if (fp->f_mode & FMODE_WRITE) {
			snprintf(strbuffer, sizeof(strbuffer), "v%s %s\n", QWLAN_VERSIONSTR, riva_version);
			if (vfs_write(fp, strbuffer, strlen(strbuffer), &fp->f_pos) < 0)
				SEC_LOG("%s: can't write file : %s", __func__, SEC_VERSION_FILEPATH);
			else
				ret = 1;
		}
	}
	if (fp && (!IS_ERR(fp)))
		filp_close(fp, NULL);
	set_fs(oldfs);
	return ret;
}
#endif /* SEC_WRITE_VERSION_IN_FILE */
