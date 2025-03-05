#include "device_tree.h"
#include "utils.h"

void *dtb_addr;

void fdt_traverse(int (*func)(void*, char*))
{
    struct fdt_header *hdr = (struct fdt_header*)dtb_addr;
    void *p;
    bool found = false;
    bool end = false;
    
    if (big2host(hdr->magic) != 0xd00dfeed)
        err("Invalid .dtb file\r\n");

    p = dtb_addr + big2host(hdr->off_dt_struct);

    while (!found && !end)
    {
        struct fdt_node_comp *ptr = (struct fdt_node_comp*)p;

        switch (big2host(ptr->token))
        {
            case FDT_BEGIN_NODE:
            {
                char name[32];
                int i;

                for (i = 0; ptr->data[i] != '\0' && ptr->data[i] != '@'; i++)
                    name[i] = ptr->data[i];
                name[i] = '\0';

                found = func((void*)ptr, name);
                if (!found)
                {
                    uint32_t i;
                    for (i = 0; ptr->data[i] != '\0'; i++) ;
                    p += (sizeof(uint32_t) + (++i) + 3) & ~3;
                }
                break;
            }
            case FDT_PROP:
            {
                struct fdt_node_prop *prop = (struct fdt_node_prop*)ptr->data;
                char *name = (char*)(dtb_addr + big2host(hdr->off_dt_strings) + big2host(prop->nameoff));

                found = func((void*)ptr, name);
                if (!found)
                {
                    uint32_t total_len = sizeof(uint32_t) + sizeof(struct fdt_node_prop) + big2host(prop->len);
                    p += (total_len + 3) & ~3;
                }
                break;
            }
            case FDT_END_NODE:
            case FDT_NOP:
                p += sizeof(uint32_t);
                break;
            case FDT_END:
                found = func((void*)ptr, "");
                end = true;
                break;
            default:
                err("Invalid fdt token\r\n");
                break;
        }
    }
}