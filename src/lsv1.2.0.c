/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
void print_permissions(mode_t mode);
int get_terminal_width(void);

int main(int argc, char const *argv[])
{
    int opt;
    int long_format = 0;

    while ((opt = getopt(argc, (char * const *)argv, "l")) != -1)
    {
        switch (opt)
        {
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If no directory arguments, default to current directory
    if (optind == argc)
    {
        if (long_format)
            do_ls_long(".");
        else
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            if (long_format)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }

    return 0;
}

void do_ls(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;
    int max_len = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        filenames = realloc(filenames, sizeof(char *) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;
        file_count++;
    }
    closedir(dp);

    if (file_count == 0)
        return;


    int term_width = get_terminal_width();
    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (file_count + num_cols - 1) / num_cols;

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int idx = col * num_rows + row;
            if (idx < file_count)
                printf("%-*s", col_width, filenames[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < file_count; i++)
        free(filenames[i]);
    free(filenames);
}

void do_ls_long(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    long total_blocks = 0;
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1)
            continue;

        total_blocks += st.st_blocks;
    }

    if (errno != 0)
        perror("readdir failed");

    rewinddir(dp);
    printf("total %ld\n", total_blocks / 2);

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1)
        {
            perror("stat failed");
            continue;
        }

        print_permissions(st.st_mode);
        printf(" %3ld ", (long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        printf("%-8s %-8s ", 
            pw ? pw->pw_name : "unknown", 
            gr ? gr->gr_name : "unknown");

        printf("%8ld ", (long)st.st_size);

        char timebuf[64];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
        printf("%s ", timebuf);

        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

void print_permissions(mode_t mode)
{
    if (S_ISDIR(mode))  printf("d");
    else if (S_ISLNK(mode)) printf("l");
    else printf("-");

    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
        return 80; // default
    return w.ws_col;
}
