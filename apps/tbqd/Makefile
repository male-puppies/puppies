include $(TOPDIR)/rules.mk

PKG_NAME:=tbqd
PKG_VERSION:=2015-08-06
PKG_RELEASE=1

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_MAINTAINER:=John Crispin <blogic@openwrt.org>

include $(INCLUDE_DIR)/package.mk

define Package/tbqd
  SECTION:=Applications
  CATEGORY:=Puppies
  TITLE:=Generic tbqd user space scripts & htmls 
endef

define Package/tbqd/description
  tolken buffer queue traffic control user scripts
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	-$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	true
endef

define Package/tbqd/install
	echo $(1) $(2)
	$(CP) -a ./files/* $(1)/
endef

define Package/tbqd/postinst
if [ -z "$${IPKG_INSTROOT}" ]; then
	echo "Enabling rc.d symlink for tbq"
	/etc/init.d/tbqd enable
fi
exit 0
endef

define Package/tbqd/prerm
if [ -z "$${IPKG_INSTROOT}" ]; then
	echo "Removing rc.d symlink for tbqd"
	/etc/init.d/tbqd disable
fi
exit 0
endef

$(eval $(call BuildPackage,tbqd))

