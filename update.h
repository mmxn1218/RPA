/*
 * update.h —— RPA 自动更新（照搬 MES_生产日报 / IT工具箱的成熟模式）
 *
 * 模式说明（与 MES_生产日报 一致）：
 *   - 远端仓库根目录放两个文件：version.json + RPA.dat
 *     （.dat 而不是 .exe：jsDelivr 对 .exe 返回 403，对 .dat 正常；
 *      同时仓库里也保留一份同名 RPA.exe，给旧客户端 / 直接下载用）
 *   - version.json 内容：{"version":"YYYY.MM.DD.HHMM","md5":"...","size":N}
 *   - 客户端 GET version.json -> 比版本号 -> 有新版就 GET RPA.dat ->
 *     校验 md5 + 字节数 -> 自我复制式替换 -> 重启
 *
 * 三处刻意设计（与 IT工具箱 同源，理由见各自注释）：
 *   1. 替换方式：自我复制式更新器（主程序复制成 _rpa_updater.exe，带
 *      --apply-update <pid> "<exe>" 启动它完成替换），不用 bat / cmd.exe。
 *   2. 版本发现：先走 GitHub API 拿 commit sha，再按 @<sha> 寻址，
 *      绕开 jsDelivr 对 @main 分支的 12 小时缓存，做到「发版即生效」。
 *      同时 fastly 主 + gcore 备，主地址被墙自动换备用。
 *   3. 失败不静默：检查/下载/替换每一步都写 RPA_update.log，
 *      真正的失败（下载或替换出错）弹窗告知，绝不悄悄吞掉。
 */
#ifndef RPA_UPDATE_H
#define RPA_UPDATE_H

#include <stddef.h>

/* ============ 发新版本时，改这里（必须和仓库 version.json 的 version 一致） ============ */
/* 版本号格式 YYYY.MM.DD.HHMM */
#define RPA_VERSION          "2026.09.30.1800"

/* 把窄字面量变成宽字面量，省得两处各写一遍、日后改一个忘一个 */
#define RPA_WIDEN2(x)        L##x
#define RPA_WIDEN(x)         RPA_WIDEN2(x)
#define RPA_VERSION_W        RPA_WIDEN(RPA_VERSION)

/* 默认更新地址（程序里写死，日后换仓库/换镜像需改这里并重编）
 *
 * 为什么用 fastly / gcore 而不是 cdn.jsdelivr.net —— 国内网络实测 cdn 入口
 * 常被 SNI 过滤（6 次全 RST），而 fastly / gcore / testingcf 稳定。
 * 四个入口背后是同一份内容，只是换了条路。工厂可指向自建内网镜像。 */
#define UPD_DEFAULT_BASE    "https://fastly.jsdelivr.net/gh/mmxn1218/RPA@main/"

/* 备用地址：主地址不通时自动试这个（同样刻意不用 raw.githubusercontent.com，
 * 实测完全不通，拿它当备用等于没有备用） */
#define UPD_FALLBACK_BASE   "https://gcore.jsdelivr.net/gh/mmxn1218/RPA@main/"

/* 第三个通道：GitHub 官方 API（只读一个 commit sha），解决 @main 的 12 小时分支缓存。
 * 公开仓库、匿名即可，不需要任何凭据。 */
#define UPD_API_COMMIT      "https://api.github.com/repos/mmxn1218/RPA/commits/main"

/* ============================================ */

#define UPD_VERSION_LEAF    "version.json"
#define UPD_DAT_LEAF        "RPA.dat"             /* 仓库里的文件名（ASCII，避开 .exe 的 403） */

/* 本地临时文件名一律用 ASCII，避开编码麻烦 */
#define UPD_TMP_LEAF        L"_rpa_new.bin"        /* 下载下来的新版本 */
#define UPD_OLD_LEAF        L"_rpa_old.exe"        /* 被替换下来的旧版本 */
#define UPD_HELPER_LEAF     L"_rpa_updater.exe"    /* 更新器副本 */
#define UPD_RESULT_LEAF     L"_rpa_update.txt"     /* 更新结果，供下次启动读取 */

typedef struct {
    char      version[32];
    char      md5[33];
    long long size;
} UpdInfo;

/* 从 "...\xxx.exe" 剥出 "...\"（带结尾反斜杠）。没有反斜杠则给空串。 */
void upd_dir_of(const wchar_t *path, wchar_t *dir, int dircch);

/* 解析 version.json（手写小解析器，不用 cJSON）。成功 0 */
int upd_parse_json(const char *json, UpdInfo *out, char *err, int errsz);

/* 版本比较：a>b 返回 1，相等 0，a<b 返回 -1。按 YYYY.MM.DD.HHMM 四段比。 */
int upd_cmp_version(const char *a, const char *b);

/* 拼 URL：base + '/' + leaf，leaf 里的非 ASCII 按 UTF-8 百分号编码 */
void upd_join_url(const char *base, const char *leaf_utf8, char *out, int outsz);

/* 检查更新。0 = 已是最新；1 = 有新版本（填 info）；<0 = 出错 */
int upd_check(const char *base, const char *local_ver, UpdInfo *info,
              char *err, int errsz);

int upd_check_ex(const char *base, const char *local_ver, UpdInfo *info,
                 char *used_base, int used_sz, char *err, int errsz);

int upd_check2(const char *base, const char *fallback, const char *local_ver,
               UpdInfo *info, char *used_base, int used_sz, char *err, int errsz);

/* 从 GitHub commits API 的 JSON 里取最新 commit sha（40 位十六进制小写） */
int upd_parse_commit_sha(const char *json, char *sha_out, int shasz);

/* 把 base 尾段里的 @引用 换成 @<sha> */
void upd_pin_commit(const char *base, const char *sha, char *out, int outsz);

/* GET api_url，解析出最新 commit。成功 0 / 失败 -1 */
int upd_resolve_commit(const char *api_url, char *sha_out, int shasz,
                       char *err, int errsz);

/* 客户端平时用的入口：先解析最新 commit 钉死地址，再走主/备地址检查。
 * 失败（网络等）也不致命 —— 最多滞后到下次能连通，但绝不能因此不给更新。 */
int upd_check4(const char *api_url, const char *base, const char *fallback,
               const char *local_ver, UpdInfo *info,
               char *used_base, int used_sz,
               char *commit_out, int commit_sz,
               char *err, int errsz);

/* 下载到 <dest_dir>\_rpa_new.bin 并校验字节数 + md5。成功 0，失败<0 */
int upd_download(const char *base, const UpdInfo *info, const wchar_t *dest_dir,
                 char *err, int errsz);

/* 复制自身成更新器并拉起它（调用方随后必须立刻退出程序）。成功 0 */
int upd_launch_helper(const wchar_t *exe_path, char *err, int errsz);

/* 由更新器调用。等旧进程退出 -> 换文件 -> 启动新版本。成功 0 */
int upd_do_apply(unsigned long pid, const wchar_t *exe_path,
                 char *err, int errsz);

/* 启动时清理上次更新留下的临时文件。返回清掉的文件数 */
int upd_cleanup(const wchar_t *dest_dir);

/* 读取上次更新的结果（若存在）。ok=1 成功，ok=0 失败并填 msg。
 * 返回 1 表示有结果可读，0 表示没有。读完会删掉结果文件。 */
int upd_read_last_result(const wchar_t *dest_dir, int *ok, char *msg_u8, int msgsz);

#endif /* RPA_UPDATE_H */
