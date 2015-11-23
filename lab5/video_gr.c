#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "math.h"

#include "video_gr.h"
#include "constants.h"
#include "vbe.h"

/* Private global variables */

static char *video_mem; /* Process address to which VRAM is mapped */
static char *phys_addr; /* Stores physical address of VRAM */

static unsigned h_res; /* Horizontal screen resolution in pixels */
static unsigned v_res; /* Vertical screen resolution in pixels */
static unsigned bits_per_pixel; /* Number of VRAM bits per pixel */

static unsigned int video_mem_size;
static unsigned int bytes_per_pixel;

static char *double_buffer;

void *vg_init(unsigned short mode) {
	struct reg86u r;
	r.u.w.ax = 0x4F02; // VBE call, function 02 -- set VBE mode
	r.u.w.bx = 1 << 14 | mode; // set bit 14: linear framebuffer
	r.u.b.intno = 0x10;

	vbe_mode_info_t vbe_mode_info;
	if (sys_int86(&r) != OK) {
		printf("set_vbe_mode: sys_int86() failed \n");
	} else if (r.u.w.ax == VBE_FUNCTION_SUPPORTED | VBE_FUNCTION_CALL_SUCCESSFUL) {
		if (vbe_get_mode_info(mode, &vbe_mode_info)) {
			return NULL;
		} else {
			h_res = vbe_mode_info.XResolution;
			v_res = vbe_mode_info.YResolution;
			bits_per_pixel = vbe_mode_info.BitsPerPixel;

			//Allow memory mapping
			struct mem_range mr;
			unsigned mr_size;
			mr.mr_base = vbe_mode_info.PhysBasePtr;
			mr_size = h_res * v_res * bits_per_pixel;
			mr.mr_limit = mr.mr_base + mr_size;

			if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))
				return NULL;

			//Map memory
			video_mem = vm_map_phys(SELF, (void *) mr.mr_base, mr_size);

			if (video_mem != MAP_FAILED) {
				if ((double_buffer = malloc(h_res * v_res * bits_per_pixel / 8))
						!= NULL) {
					return video_mem;
				}
			}
		}
	}
	return NULL;
}

int vg_exit() {
	struct reg86u reg86;

	reg86.u.b.intno = 0x10; /* BIOS video services */
	reg86.u.b.ah = 0x00; /* Set Video Mode function */
	reg86.u.b.al = 0x03; /* 80x25 text mode*/

	if (sys_int86(&reg86) != OK) {
		printf("\tvg_exit(): sys_int86() failed \n");
		return 1;
	} else
		return 0;
}

char* getVideoMem() {

	return video_mem;
}

unsigned getHRes() {

	return h_res;
}

unsigned getVRes() {

	return v_res;
}

void swap(unsigned short* xi, unsigned short* xf) {
	unsigned short temp = *xi;
	*xi = *xf;
	*xf = temp;
}

//Round function, introduced in Minix 3.3.0
double round(double x) {
	double t;
	if (x >= 0.0) {
		t = ceil(x);
		if (t - x > 0.5)
			t -= 1.0;
		return (t);
	} else {
		t = ceil(-x);
		if (t + x > 0.5)
			t -= 1.0;
		return (-t);
	}
}
int clearPixmap(int xi, int yi, int width, int height) {
	if ((xi >= h_res || xi < 0) || (yi >= v_res || yi < 0)) {
		printf("ERROR: Invalid coordinates!\n");
		return EXIT_FAILURE;
	}
	int line, column;
	for (line = 0; line < height; line++) {
		for (column = 0; column < width; column++) {
			if (xi + column < h_res || h_res * (yi + line) < v_res) {
				*(double_buffer + (h_res * (yi + line))
						+ (xi + column) * bits_per_pixel / 8) = 0x00;
			}
		}
	}
	memcpy(video_mem, double_buffer, h_res * v_res * bits_per_pixel / 8);

	return EXIT_SUCCESS;
}
int drawPixmap(int xi, int yi, char* pixmap, int width, int height) {
	if ((xi >= h_res || xi < 0) || (yi >= v_res || yi < 0)) {
		printf("ERROR: Invalid coordinates!\n");
		return EXIT_FAILURE;
	}

	int line, column;
	for (line = 0; line < height; line++) {
		for (column = 0; column < width; column++) {
			if (xi + column < h_res || h_res * (yi + line) < v_res) {
				*(double_buffer + (h_res * (yi + line))
						+ (xi + column) * bits_per_pixel / 8) = pixmap[column
						+ (line * width)];
			}
		}
	}
	memcpy(video_mem, double_buffer, h_res * v_res * bits_per_pixel / 8);

	return EXIT_SUCCESS;
}
