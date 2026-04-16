#!/bin/bash

# 自动修改config.ini文件中的IP地址
# 用法: ./update_ip.sh [可选:IP地址]

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 备份目录
BACKUP_DIR="bak_`date '+%Y%m%d_%H%M%S'`"

# 配置文件查找路径
INSTALL_DIR=/home/gb/USB-Device-Mgr/LibToUSB-Device-Mgr/

# 打印彩色消息
print_color() {
    echo -e "${1}${2}${NC}"
}

# 验证IP地址格式
validate_ip() {
    local ip=$1
    local stat=1

    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        # 3. 检查每部分是否在0-255之间
        if [[ ${ip[0]} -ge 0 && ${ip[0]} -le 255 ]] && \
           [[ ${ip[1]} -ge 0 && ${ip[1]} -le 255 ]] && \
           [[ ${ip[2]} -ge 0 && ${ip[2]} -le 255 ]] && \
           [[ ${ip[3]} -ge 0 && ${ip[3]} -le 255 ]]; then
            stat=0
        fi
    fi
    return $stat
}

# 获取本机IP地址
get_local_ip() {
    # 方法1: 通过DNS获取
    local ip
    ip=$(hostname -I 2>/dev/null | awk '{print $1}')

    if [ -n "$ip" ]; then
        echo "$ip"
        return 0
    fi

    return 1
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项] [IP地址]"
    echo ""
    echo "选项:"
    echo "  -h, --help      显示此帮助信息"
    echo "  -i, --ip IP     指定要设置的IP地址"
    echo "  -l, --list      只显示找到的配置文件，不修改"
    echo "  -v, --version   显示版本信息"
    echo ""
    echo "示例:"
    echo "  $0              # 自动获取IP并修改"
    echo "  $0 192.168.1.100 # 指定IP地址"
    echo "  $0 --ip 10.0.0.1 # 指定IP地址"
    echo "  $0 --list       # 只显示文件列表"
    echo ""
    exit 0
}

# 显示版本
show_version() {
    echo "IP地址修改工具 v1.0"
    exit 0
}

# 显示文件列表
show_file_list() {
    print_color "当前目录下找到的config.ini文件:" "$BLUE"
    echo ""

    local count=0
    while IFS= read -r -d '' file; do
        ((count++))
        echo " $count. $file"

        # 显示当前配置
        if grep -q "adapter-server" "$file" 2>/dev/null; then
            config_line=$(grep "adapter-server" "$file" | head -1)
            echo "    当前配置: $config_line"
        fi
    done < <(find ${INSTALL_DIR} -type f -name "config.ini" ! -path "*/.*" -print0)

    echo ""
    print_color "总共找到 $count 个文件" "$BLUE"
    exit 0
}

# 查找并修改配置文件
update_config_files() {
    local new_ip=$1
    local updated=0
    local failed=0

    print_color "开始查找config.ini文件..." "$BLUE"

    # 使用find查找所有config.ini文件
    while IFS= read -r -d '' file; do
        print_color "处理文件: $file" "$YELLOW"

        # 备份原文件
        if [ ! -f "${file}.${BACKUP_DIR}" ]; then
            cp "$file" "${file}.${BACKUP_DIR}"
        fi

        # 使用sed修改IP地址
        if sed -i -E "s/(adapter-server[[[:space:]]*=[[:space:]]*)[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/\1${new_ip}/" "$file" 2>/dev/null; then
            # 检查是否真的修改了
            if grep -q "adapter-server.*$new_ip" "$file"; then
                old_config=$(grep "adapter-server" "${file}.${BACKUP_DIR}")
                new_config=$(grep "adapter-server" "$file")
                print_color "修改前: $old_config" "$YELLOW"
                print_color "修改后: $new_config" "$GREEN"

                # 删除临时备份文件
                rm -f "${file}.bak"
                ((updated++))
            else
                print_color "警告: 文件已处理但未找到adapter-server字段" "$YELLOW"
            fi
        else
            print_color "错误: 无法修改文件 $file" "$RED"
            ((failed++))
        fi
    done < <(find ${INSTALL_DIR} -type f -name "config.ini" ! -path "*/.*" -print0)

    echo ""
    print_color "修改完成！" "$BLUE"
    print_color "成功修改: $updated 个文件" "$GREEN"
    print_color "修改失败: $failed 个文件" $(if [ $failed -gt 0 ]; then echo "$RED"; else echo "$GREEN"; fi)

    if [ $updated -gt 0 ]; then
        print_color "原文件已备份为 .${BACKUP_DIR} 文件" "$BLUE"
    fi
}

# 主函数
main() {
    print_color "=============================================" "$BLUE"
    print_color " 自动修改IP地址配置工具" "$GREEN"
    print_color "=============================================" "$BLUE"
    echo ""

    # 解析参数
    local custom_ip=""
    local action="update"

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                ;;
            -v|--version)
                show_version
                ;;
            -l|--list)
                action="list"
                ;;
            -i|--ip)
                if [[ -n "$2" ]]; then
                    custom_ip="$2"
                    shift
                else
                    print_color "错误: --ip 参数需要一个IP地址" "$RED"
                    exit 1
                fi
                ;;
            *)
                # 如果没有前缀，且看起来像IP地址，则当作IP参数处理
                if [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                    custom_ip="$1"
                else
                    print_color "错误: 未知参数 $1" "$RED"
                    show_help
                fi
                ;;
        esac
        shift
    done

    # 如果指定了list操作
    if [ "$action" = "list" ]; then
        show_file_list
    fi

    # 获取或使用指定的IP地址
    local new_ip=""

    # 如果字符串不为空
    if [ -n "$custom_ip" ]; then
        if validate_ip "$custom_ip"; then
            new_ip="$custom_ip"
            print_color "使用指定的IP地址: $new_ip" "$GREEN"
        else
            print_color "错误: IP地址格式不正确: $custom_ip" "$RED"
            exit 1
        fi
    else
        print_color "正在获取本机IP地址..." "$BLUE"
        # $(get_local_ip) 是用来捕获函数的输出，并不是获取函数返回值，函数的返回值通过 $? 获取
        # 这里的 if 会判断命令 get_local_ip 的退出码，如果为 0 才会执行 then 块
        new_ip=$(get_local_ip)
        if [ $? -eq 0 ]; then
            print_color "检测到本机IP地址: $new_ip" "$GREEN"
        else
            print_color "警告: 无法自动获取IP地址" "$YELLOW"
            read -p "请输入要设置的IP地址: " new_ip

            if [ -z "$new_ip" ]; then
                print_color "错误: 未输入IP地址" "$RED"
                exit 1
            fi

            if ! validate_ip "$new_ip"; then
                print_color "错误: IP地址格式不正确: $new_ip" "$RED"
                exit 1
            fi
        fi
    fi

    echo ""
    # 显示当前目录
    print_color "当前目录: $(pwd)" "$BLUE"

    # 查找配置文件
    local file_count
    file_count=$(find ${INSTALL_DIR} -type f -name "config.ini" ! -path "*/.*" 2>/dev/null | wc -l)

    if [ "$file_count" -eq 0 ]; then
        print_color "未找到任何config.ini文件" "$YELLOW"
        exit 0
    fi

    print_color "找到 $file_count 个config.ini文件" "$BLUE"
    echo ""

    # 确认是否继续
    if [ "$action" = "update" ]; then
        read -p "是否要继续修改这些文件？(y/N): " confirm
        confirm=${confirm:-N}

        if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
            print_color "操作已取消" "$YELLOW"
            exit 0
        fi

        # 开始修改文件
        update_config_files "$new_ip"
    fi
}

# 脚本入口
if [[ "${BASH_SOURCE[0]}" = "$0" ]]; then
    main "$@"
fi