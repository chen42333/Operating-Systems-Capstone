#include "file.h"
#include "process.h"
#include "vfs.h"
#include "uart.h"

int open(const char *pathname, int flags) {
    struct file *f;
    struct pcb_t *pcb = get_current();
    int idx;

    if (~pcb->fd_bitmap == 0) {
        err("Fd table is full\r\n");
        return -1;
    }

    vfs_open(pathname, flags, &f);
    idx = __builtin_ctz(~pcb->fd_bitmap);
    pcb->fd_table[idx] = f;
    pcb->fd_bitmap |= (1ULL << idx);

    return idx;
}

int close(int fd) {
    struct pcb_t *pcb = get_current();
    struct file *f = pcb->fd_table[fd];

    if (!f) {
        err("Invalid fd\r\n");
        return -1;
    }

    pcb->fd_table[fd] = NULL;
    pcb->fd_bitmap &= ~(1ULL << fd);

    return vfs_close(f);
}

long write(int fd, const void *buf, unsigned long count) {
    struct file *f = get_current()->fd_table[fd];

    return vfs_write(f, buf, count);
}

long read(int fd, void *buf, unsigned long count) {
    struct file *f = get_current()->fd_table[fd];

    return vfs_read(f, buf, count);
}

int mkdir(const char *pathname, unsigned mode) {
    struct vnode *node;

    if (vfs_mkdir(pathname, &node) < 0 || !node)
        return -1;

    node->mode = mode;    

    return 0;
}

int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data) {
    if (vfs_mount(target, filesystem, flags) < 0)
        return -1;

    return 0;
}

int chdir(const char *path) {
    struct pcb_t *pcb = get_current();
    struct vnode *parent_vnode, *node;
    char component[STRLEN];

    if (find_parent_vnode(path, &node, &parent_vnode, component) < 0)
        return -1;

    pcb->cur_dir = node;

    return 0;
}

long lseek64(int fd, long offset, int whence) {
    struct file *f = get_current()->fd_table[fd];

    return vfs_lseek64(f, offset, whence);
}

void ls(char *dirname) {
    struct vnode *node, *parent_node;
    char component[STRLEN];

    if (!dirname)
        dirname = get_current()->cur_dir->name;

    if (find_parent_vnode(dirname, &node, &parent_node, component) < 0 || !node) {
        err("No such directory\r\n");
        return;
    }

    parent_node = node;

    for (struct node *tmp = parent_node->children.head; tmp; tmp = tmp->next) {
        node = tmp->ptr;
        if (!node->hidden)
            printf("%s\r\n", node->name);
    }
}

int cat(char *filename) {
    struct vnode *node, *parent_node;
    char component[STRLEN];

    if (find_parent_vnode(filename, &node, &parent_node, component) < 0 || !node) {
        err("File not exists\r\n");
        return -1;
    }

    uart_write(node->content, node->file_size);
    printf("\r\n");

    return 0;
}


void* load_prog(char *filename, size_t *prog_size) {
    struct vnode *node, *parent_node;
    char component[STRLEN];
    void *prog_addr;

    if (find_parent_vnode(filename, &node, &parent_node, component) < 0 || !node) {
        err("File not exists\r\n");
        return NULL;
    }

    prog_addr = malloc((node->file_size / PAGE_SIZE + (node->file_size % PAGE_SIZE > 0)) * PAGE_SIZE);
    if (!prog_addr)
        return NULL;
        
    *prog_size = node->file_size;
    for (int i = 0; i < *prog_size; i++)
        *((char*)prog_addr + i) = *(node->content + i);
    
    return prog_addr;
}