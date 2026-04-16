#!/bin/bash
#set -x

INSTALL_DIR=/home/gb/USB-Device-Mgr/LibToUSB-Device-Mgr/
PROG_NAME=Test_LibToUSB-Device-Mgr
SYSLIB_INSTALL_DIR=/usr/lib/
SYSINCLUDE_INSTALL_DIR=/usr/include/
SYSCONF_INSTALL_DIR=/userdata/config/libtousb-device-mgr/
CONF_INI=g_libtousb.conf

#备份
BACKUP_DIR="bak_`date '+%Y%m%d_%H%M%S'`"

#单板目录
CPSYNC_DIR=bin_cpsync
CPPEM_DIR=bin_cppem
CPDPS_DIR=bin_cpdps
CPRCA_DIR=bin_cprca
FTSYNC_DIR=bin_ftsync
FTPPS_DIR=bin_ftpps
FTPGB_DIR=bin_ftpgb
FTASIC_DIR=bin_ftasic
CONF_NAME=config.ini

backup_install_dir()
{
if [ -d ${INSTALL_DIR} ]
then
    # 删除之前备份目录
    rm -rf ${INSTALL_DIR}/bak_*

    # 建立新的备份目录
    # BACKUP_DIR="bak_`date '+%Y%m%d_%H%M%S'`"
    mkdir -p ${INSTALL_DIR}/${BACKUP_DIR}

    #将当前安装目录内容备份到新的备份目录
    cp -r ${INSTALL_DIR}/include  ${INSTALL_DIR}/${BACKUP_DIR}
    cp -r ${INSTALL_DIR}/lib     ${INSTALL_DIR}/${BACKUP_DIR}
    cp -r ${INSTALL_DIR}/bin*     ${INSTALL_DIR}/${BACKUP_DIR}

    #删除安装目录下所有文件及目录(当前新建的备份目录除外)
    find ${INSTALL_DIR} -mindepth 1 -maxdepth 1 ! -name "${BACKUP_DIR}" -exec rm -rf {} +
fi
}

make_install_dir()
{
if [ ! -d ${INSTALL_DIR} ]
then
    mkdir -p ${INSTALL_DIR}
else
    backup_install_dir
fi

if [ ! -d ${SYSCONF_INSTALL_DIR} ]
then
    mkdir -p ${SYSCONF_INSTALL_DIR}
else
    #备份/userdata/config/libtousb-device-mgr目录的配置文件
    cp ${SYSCONF_INSTALL_DIR}/${CONF_INI}  ${SYSCONF_INSTALL_DIR}/${CONF_INI}.${BACKUP_DIR}
fi

mkdir -p ${INSTALL_DIR}/include
mkdir -p ${INSTALL_DIR}/lib
mkdir -p ${INSTALL_DIR}/bin
mkdir -p ${INSTALL_DIR}/script
}

copy_install_file()
{
if [ -d "include" ]
then
    cp -r ./include ${INSTALL_DIR}/
fi

if [ -d "lib" ]
then
    cp -r ./lib ${INSTALL_DIR}/
fi

if [ -d "bin" ]
then
    cp -r ./bin ${INSTALL_DIR}/
fi

if [ -d "script" ]
then
    cp -r ./script ${INSTALL_DIR}/
fi

cp ${INSTALL_DIR}/lib/libusb.so                         ${SYSLIB_INSTALL_DIR}
cp ${INSTALL_DIR}/include/lib_interface.h                ${SYSINCLUDE_INSTALL_DIR}
cp ${INSTALL_DIR}/bin/g_libtousb.conf                    ${SYSCONF_INSTALL_DIR}

# 创建不同类型的单板目录
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${CPSYNC_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${CPPEM_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${CPDPS_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${CPRCA_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${FTSYNC_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${FTPPS_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${FTASIC_DIR}
cp -r ${INSTALL_DIR}/bin ${INSTALL_DIR}/${FTPGB_DIR}

# 替换之前的配置信息
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${CPSYNC_DIR}/${CONF_NAME}" "${INSTALL_DIR}${CPSYNC_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${CPPEM_DIR}/${CONF_NAME}" "${INSTALL_DIR}${CPPEM_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${CPDPS_DIR}/${CONF_NAME}" "${INSTALL_DIR}${CPDPS_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${CPRCA_DIR}/${CONF_NAME}" "${INSTALL_DIR}${CPRCA_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${FTSYNC_DIR}/${CONF_NAME}" "${INSTALL_DIR}${FTSYNC_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${FTPPS_DIR}/${CONF_NAME}" "${INSTALL_DIR}${FTPPS_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${FTASIC_DIR}/${CONF_NAME}" "${INSTALL_DIR}${FTASIC_DIR}/${CONF_NAME}"
replace_ini "${INSTALL_DIR}${BACKUP_DIR}/${FTPGB_DIR}/${CONF_NAME}" "${INSTALL_DIR}${FTPGB_DIR}/${CONF_NAME}"
}

chmod_file()
{
chmod 755 ${INSTALL_DIR}/bin/*.sh
chmod 755 ${INSTALL_DIR}/bin/${PROG_NAME}
chmod 755 ${INSTALL_DIR}/script/*.sh
}

replace_ini()
{
SRC_INI="$1"
DST_INI="$2"

if [ -f "${SRC_INI}" ]
then
    echo "replace_ini() old_ini=${SRC_INI}"
    echo "replace_ini() new_ini=${DST_INI}"

    while IFS='=' read -r key value; do
        # 跳过注释和空行
        if [[ "$key" =~ ^#.* ]] || [[ -z "$key" ]] || [[ -z "$value" ]]; then
            continue
        fi

        # 去除可能的空格和引号
        key=$(echo $key | tr -d '[:space:]')
        value=$(echo $value | tr -d '[:space:]' | sed "s/^[\"']//;s/['\"]$//")

        # 赋值给变量
        declare "${key}=${value}"
        sed -i "s|^${key}=.*$|${key}=${value}|g" "${DST_INI}"

        echo "replace_ini() ${key}=${value}"
    done < ${SRC_INI}

else
    echo "replace_ini() ${SRC_INI} is not existed"
fi
}

auto_install()
{
make_install_dir
copy_install_file
replace_ini "${SYSCONF_INSTALL_DIR}${CONF_INI}.${BACKUP_DIR}" "${SYSCONF_INSTALL_DIR}${CONF_INI}"
chmod_file
}

auto_install