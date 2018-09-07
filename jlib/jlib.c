/*
 * jlib.c
 *
 * @chuanjiong
 */

#include "jlib.h"
#include "jstatus.h"

volatile int exit_flag = 0;

static jtime start_time;
static jdate start_date;

static int jlib_verify(void)
{
    int i = 0x12;
    unsigned char *c = (unsigned char *)&i;
    if (*c != 0x12)
    {
        jlog("[jlib] system is big endian\n");
        return ERROR_FAIL;
    }

    int match = 0;

    char *p = NULL;
    if (sizeof(p) == 4)
    {
        jlog("[jlib] this program is compile @ 32-bit mode ");
        match = 32;
    }
    else
    {
        jlog("[jlib] this program is compile @ 64-bit mode ");
        match = 64;
    }

    char buf[32];
    FILE *fp = popen("uname -m", "r");
    fread(buf, 1, 32, fp);
    if (!strncmp(buf, "x86_64", 6))
    {
        jlog("and run @ 64-bit system\n");
        if (match == 64)
            match = 0;
        else
            match = -2;
    }
    else
    {
        jlog("and run @ 32-bit system\n");
        if (match == 32)
            match = 0;
        else
            match = -3;
    }
    pclose(fp);

    if (match != 0)
        return ERROR_FAIL;
    else
        return SUCCESS;
}

static int jprocfs_file_exit(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    jatomic_true(&exit_flag);

    return jsnprintf(buf, size, "exiting...");
}

static int jprocfs_file_memory(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    int total=0;

#if (MEM_DEBUG == 1)
    total += jmalloc_trace(buf, size);
#endif

    return total;
}

static int jprocfs_file_svn(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    return jsnprintf(buf, size, "svn %s", SVN_VER);
}

void jlib_setup(void)
{
    if (jlib_verify() != SUCCESS)
    {
        jerr("[jlib] jlib_verify fail\n");
        while (1)
            jsleep(1000);
    }

#ifdef MINGW32
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA wsaData;
    WSAStartup(sockVersion, &wsaData);
#endif

#if (MEM_DEBUG == 1)
    jmalloc_setup();
#endif

    jsocket_setup();

    jevent_setup();

    jvfs_setup();

    jprocfs_add_file("exit", jprocfs_file_exit);
    jprocfs_add_file("memory", jprocfs_file_memory);
    jprocfs_add_file("svn", jprocfs_file_svn);

    jstatus_setup();

    system("echo 2097152 > /proc/sys/net/core/rmem_max");
    system("echo 2097152 > /proc/sys/net/core/rmem_default");
    system("echo 2097152 > /proc/sys/net/core/wmem_max");
    system("echo 2097152 > /proc/sys/net/core/wmem_default");

    start_time = jtime_set_anchor();
    start_date = jdate_get_local();

    //ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
}

jbool jlib_is_exit(void)
{
    if (jatomic_get(&exit_flag))
        return jtrue;
    else
        return jfalse;
}

void jlib_shutdown(void)
{
    char buf[64] = {0};
    jdate_format_date(start_date, buf, 64, DATEFORMAT_W_DMY_HMS_GMT);
    jlog("[jlib] start date: %s\n", buf);
    jdate end = jdate_get_local();
    jdate_format_date(end, buf, 64, DATEFORMAT_W_DMY_HMS_GMT);
    jlog("[jlib] end date: %s\n", buf);

    jtime life = jtime_get_period(start_time);
    int day = life.tv_sec/(24*60*60);
    int hour = (life.tv_sec/(60*60))%24;
    int min = (life.tv_sec/60)%60;
    int sec = life.tv_sec%60;
    jlog("[jlib] life time: %d day %d hour %d min %d sec\n", day, hour, min, sec);

    jstatus_shutdown();

    jvfs_shutdown();

    jevent_shutdown();

    jsocket_shutdown();

#if (MEM_DEBUG == 1)
    jmalloc_shutdown();
#endif

#ifdef MINGW32
    WSACleanup();
#endif
}


