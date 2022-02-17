#ifdef _WIN32
#include "SDL.h"
#include "SDL_image.h"
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#include <string>
#include <cstring>
#include <iostream>
#include <cstdint>

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
bool closed = false;

// https://lazyfoo.net/tutorials/SDL/40_texture_manipulation/index.php
class Texture {
		SDL_Texture* tex;
		SDL_PixelFormat* fmt;
	public:
		int width, height;
		Texture(std::string filepath) : tex{nullptr}, width{0}, height{0} {
			filepath = "Textures/" + filepath;
			SDL_Surface* surf = IMG_Load(filepath.c_str());
			if (surf == nullptr) {
				std::cerr << "Unable to load " << filepath << std::endl;
				return;
			}
			SDL_Surface* formatted = SDL_ConvertSurfaceFormat(surf, SDL_GetWindowPixelFormat(gWindow), 0);
			SDL_FreeSurface(surf);
			if (formatted == nullptr) {
				std::cerr << "Unable to convert surface for " << filepath << " to display format: " << SDL_GetError() << std::endl;
			}
			tex = SDL_CreateTexture(gRenderer, SDL_GetWindowPixelFormat(gWindow), SDL_TEXTUREACCESS_STREAMING, formatted->w, formatted->h);
			if (tex == nullptr) {
				std::cerr << "Unable to create blank texture for " << filepath << ": " << SDL_GetError() << std::endl;
			}
			void *pixels;
			int pitch;
			SDL_LockTexture(tex, 0, &pixels, &pitch); // make texture writable (init pixels to right size)
			memcpy(pixels, formatted->pixels, formatted->pitch * formatted->h);
			SDL_UnlockTexture(tex);
			width = formatted->w;
			height = formatted->h;
			fmt = formatted->format;
			SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
			SDL_FreeSurface(formatted);
		}
		
		void render(int x, int y, int R = 0xFF, int G = 0xFF, int B = 0xFF) {
			SDL_SetRenderDrawColor(gRenderer, R, G, B, 0xFF);
			// int x, iny y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip
			SDL_Rect quad = {x, y, width, height};
			SDL_RenderCopyEx(gRenderer, tex, 0, &quad, 0.0, 0, SDL_FLIP_NONE);
		}
		
		void set_size(int new_width, int new_height) {
			width = new_width;
			height = new_height;
		}

		void set_colour(Uint8 R, Uint8 G, Uint8 B) { // also map dark -> transparent
			SDL_SetTextureColorMod(tex, R, G, B);
		}
		
		~Texture() {
			if (tex) SDL_DestroyTexture(tex);
		}
};

class Button {
		Texture* pressed;
		Texture* released;
		int x, y;
	public:
		enum Mode{Fill, Negative, Dark};
		Button(std::string name, Mode m) : pressed{nullptr}, released{nullptr}, x{0}, y{0} {
			if (m == Fill) {
				pressed = new Texture(name + "-pressed-fill.png");
			} else if (m == Negative) {
				pressed = new Texture(name + "-pressed-negative.png");
			} else if (m == Dark) {
				pressed = new Texture(name + "-pressed-dark.png");			
			} else {
				std::cerr << "Invalid colour mode loading " << name << std::endl;
				return;
			}
			released = new Texture(name + "-outline.png");
		}
		
		void press(int R = 0xFF, int G = 0xFF, int B = 0xFF) { 
			pressed->render(x, y, R, G, B);
		}
		
		void release(int R = 0xFF, int G = 0xFF, int B = 0xFF) {
			released->render(x, y, R, G, B);
		}

		void set_location(int new_x, int new_y) {
			x = new_x;
			y = new_y;
		}

		void set_colour(Uint8 R, Uint8 G, Uint8 B) {
			pressed->set_colour(R, G, B);
			released->set_colour(R, G, B);
		}
};

struct visuals {
    bool A;
    bool B;
    bool UP;
    bool DOWN;
    bool LEFT;
    bool RIGHT;
    int accel[3];
    int ir[2];
    
    bool hasNunchuk;
    bool C;
    bool Z;
    int stick[2];
};

class WiimoteLayout {
		Button A, B, C, Z;
		Texture joystick, dpad, up, down, left, right;
		int jx, jy, dpx, dpy;
	public:
		visuals v;

		WiimoteLayout() :
			A{"a", Button::Mode::Negative}, // need to give everything positions
			B{"b", Button::Mode::Negative}, // load positions from file
			C{"c", Button::Mode::Negative}, // be able to switch layouts
			Z{"z", Button::Mode::Negative},
			joystick{"joystick-gate.png"},
			dpad{"d-pad-gate.png"},
			up{"d-pad-up.png"},
			down{"d-pad-down.png"},
			left{"d-pad-left.png"},
			right{"d-pad-right.png"},
			v{}
		{
			A.set_location(290, 20);
			B.set_location(390, 20);
			C.set_location(180, 52);
			Z.set_location(180, 0);
			jx = 50;
			jy = 60;
			dpx = 180;
			dpy = 125;
			A.set_colour(100, 255, 255); // 100,255,255 P1 light blue
			B.set_colour(255,220,0); 
			// P2 yellow: 255,220,0
		}

		// center x/y and joystick x/y
		void DrawStick(int cx, int cy, int jx, int jy, int width) {
			if (width < 1) return;
			SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawLine(gRenderer,cx,cy,cx+jx,cy+jy);
			for (int i = 1; i < width; ++i) {
				// To speed up: determine quadrant of (jx,jy) and only draw lines
				// along a diagonal in the opposite quadrant
				for (int j = i; j >= 0; --j) { // draw diagonals around (cx,cy)
					SDL_RenderDrawLine(gRenderer,cx+j,cy-i+j,cx+jx+j,cy+jy-i+j); // top right
					SDL_RenderDrawLine(gRenderer,cx-j,cy+i-j,cx+jx-j,cy+jy+i-j); // bottom left
					SDL_RenderDrawLine(gRenderer,cx+j,cy+i-j,cx+jx+j,cy+jy+i-j); // bottom right
					SDL_RenderDrawLine(gRenderer,cx-j,cy-i+j,cx+jx-j,cy+jy-i+j); // top left
				}
			}
		}
		
		void Draw() {
			if (v.hasNunchuk) {
				if (v.A) { A.press(); } else { A.release(); }
				if (v.B) { B.press(); } else { B.release(); }
				if (v.C) { C.press(); } else { C.release(); }
				if (v.Z) { Z.press(); } else { Z.release(); }
				dpad.render(dpx, dpy);
				if (v.UP) up.render(dpx, dpy);
				if (v.DOWN) down.render(dpx, dpy);
				if (v.LEFT) left.render(dpx, dpy);
				if (v.RIGHT) right.render(dpx, dpy);
				joystick.render(jx, jy);
				DrawStick(jx+joystick.width/2,jy+joystick.height/2,v.stick[0],v.stick[1],3);
			} else {
				if (v.A) { A.press(); } else { A.release(); }
				if (v.B) { B.press(); } else { B.release(); }
			}
		}
		
		void poll() { // reinventing the wheel (masking polls)
			
		}
			
};

void clamp_ir(struct visuals *v) {
    if (v->ir[0] < 0) v->ir[0] = 0;
    if (v->ir[1] < 0) v->ir[1] = 0;
    if (v->ir[0] > 1024) v->ir[0] = 1024;
    if (v->ir[1] > 768) v->ir[1] = 768;
}

void clamp_acc(struct visuals *v) { // impossible to need?
    for (int i = 0; i < 3; ++i) {
        if (v->accel[i] < 0) v->accel[i] = 0;
        if (v->accel[i] > 1023) v->accel[i] = 1023;
    }
}

// also flips coords for convenient drawing
void normalize_joystick(struct visuals *v) {
	if (v->hasNunchuk) {
		v->stick[0] = (v->stick[0] - 128)/2;
		v->stick[1] = (127- v->stick[1])/2;
	}
}

// assumes it's given an expected report
// key length = 16
void parse_report(struct visuals *v, const uint8_t *buf, const uint8_t *key) {
    v->LEFT = buf[2] & 0x01;
    v->RIGHT = buf[2] & 0x02;
    v->DOWN = buf[2] & 0x04;
    v->UP = buf[2] & 0x08;
    v->B = buf[3] & 0x04;
    v->A = buf[3] & 0x08;
    v->accel[0] = (buf[4] << 2) + ((buf[2] & 0x60) >> 5);
    v->accel[1] = (buf[5] << 2) + ((buf[3] & 0x20) >> 4);
    v->accel[2] = (buf[6] << 2) + ((buf[3] & 0x40) >> 5);
    v->ir[0] = 0;
    v->ir[1] = 0;
    if (buf[1] == 0x33) {
        v->hasNunchuk = false;
        for (int i = 0; i < 4; i++) { // 12 byte IR
            v->ir[0] += buf[7 + i * 2] + ((buf[9 + i * 2] & 0x30) << 8);
            v->ir[1] += buf[8 + i * 2] + ((buf[9 + i * 2] & 0xC0) << 8);
        }
    } else if (buf[1] == 0x37) {
        v->hasNunchuk = true;
        for (int i = 0; i < 2; i++) { // 10 byte IR
            v->ir[0] += buf[7 + i * 5] + ((buf[9 + i * 5] & 0x30) << 8);
            v->ir[1] += buf[8 + i * 5] + ((buf[9 + i * 5] & 0xC0) << 8);
            v->ir[0] += buf[10 + i * 5] + ((buf[9 + i * 5] & 0x03) << 8);
            v->ir[1] += buf[11 + i * 5] + ((buf[9 + i * 5] & 0x0C) << 8);
        }
        // ((data[i] ^ key[8 + i]) + key[i]) % 0x100
        v->stick[0] = ((buf[17] ^ key[8]) + key[0]) % 0x100;
        v->stick[1] = ((buf[18] ^ key[9]) + key[1]) % 0x100;
        uint8_t decrypted_final = ((buf[22] & key[13]) + key[5]) % 0x100;
        v->Z = (decrypted_final & 0x1) == 0;
        v->C = (decrypted_final & 0x2) == 0;
    }
    clamp_ir(v);
	clamp_acc(v);
	normalize_joystick(v);
}

// returns true if the report is either nunchuk data or no extension w/IR
bool is_input_report(const uint8_t * buf, int len) {
    return (
		buf != nullptr &&
		len > 0 &&
        buf[0] == 0xA1 &&
        (
            (len == 0x17 && buf[1] == 0x37) ||
            (len == 0x13 && buf[1] == 0x33)
        )
    );/* {
    	return true;
    }
    printf("bad report\n");
    return false;*/
}

void print_inputs(struct visuals *v) {
    printf("IR:%04d,%03d ACC:%04d,%04d,%04d ", v->ir[0], v->ir[1], v->accel[0], v->accel[1], v->accel[2]);
    if (v->hasNunchuk) printf("x:%4d y:%4d ", v->stick[0], v->stick[1]);
    if (v->A) printf("A ");
    if (v->B) printf("B ");
    if (v->UP) printf("UP ");
    if (v->DOWN) printf("DOWN ");
    if (v->LEFT) printf("LEFT ");
    if (v->RIGHT) printf("RIGHT ");
    if (v->hasNunchuk) {
        if (v->Z) printf("Z ");
        if (v->C) printf("C ");
    }
    printf("\n");
}

uint8_t extension_decryption_key[16] = {0x0};
uint8_t temp_key[12] = {0x0};
enum KeyState {Uninitialized, EncryptionEnabled, Block1, Block2, Complete};
int state = Uninitialized;

// https://wiibrew.org/wiki/Wiimote/Protocol#Extension_Controllers
void store_extension_key(const uint8_t *buf, int len) { // output report (from wii) buf[0]=a2
	if (len < 1 + 6 + 16) return; // a2 16 MM FF FF FF SS + key
	if (state == Uninitialized) printf("%02X %02X %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	if (buf[1] != 0x16) return; // write memory register
	if (buf[2] != 0x04) return; // enable write data
	if (buf[3] != 0xA4 || buf[4] != 0x00) return; // register choice
	if (state == Uninitialized) {
		if (buf[5] == 0xF0 && buf[6] == 0x01 && buf[7] == 0xAA) {
			state = EncryptionEnabled;
			printf("ENABLE ENCRYPTION\n");
		}
	} else if (state == EncryptionEnabled) {
		if (buf[5] == 0x40 && buf[6] == 0x06) {
			temp_key[0] = buf[7];
			temp_key[1] = buf[8];
			temp_key[2] = buf[9];
			temp_key[3] = buf[10];
			temp_key[4] = buf[11];
			temp_key[5] = buf[12];
			state = Block1;
			printf("KEY BLOCK1\n");
		} else {
			state = Uninitialized;
		}
	} else if (state == Block1) {
		if (buf[5] == 0x46 && buf[6] == 0x06) {
			temp_key[6] = buf[7];
			temp_key[7] = buf[8];
			temp_key[8] = buf[9];
			temp_key[9] = buf[10];
			temp_key[10] = buf[11];
			temp_key[11] = buf[12];
			state = Block2;
			printf("KEY BLOCK2\n");
		} else {
			state = Uninitialized;
		}
	} else if (state == Block2) {
		if (buf[5] == 0x4C && buf[6] == 0x04) {
			memcpy(extension_decryption_key, temp_key, sizeof(uint8_t)*12);
			extension_decryption_key[12] = buf[7];
			extension_decryption_key[13] = buf[8];
			extension_decryption_key[14] = buf[9];
			extension_decryption_key[15] = buf[10];
			state = Uninitialized; // allow it to check again
			printf("KEY COMPLETE\n");
			for (int i = 0; i < 8; ++i) printf("%02X ", extension_decryption_key[i]);
			printf("\n");
			for (int i = 8; i < 16; ++i) printf("%02X ", extension_decryption_key[i]);
			printf("\n");
		}
	}
}

void visualize_inputs_console(const uint8_t *buf, int len) {//, const unit8_t *key) {
    if (!is_input_report(buf, len)) return;
    struct visuals v;
    parse_report(&v, buf, extension_decryption_key);
    print_inputs(&v);
}

WiimoteLayout *wm = nullptr;

int init_visualizer(void) {
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(550, 250, 0, &gWindow, &gRenderer);
	SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_ADD);
	IMG_Init(IMG_INIT_PNG);
	wm = new WiimoteLayout();
	return 1;
}

void exit_visualizer() {
	if (closed) return;
	closed = 1;
	delete wm;
	IMG_Quit();
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	SDL_Quit();
}

void all_on(struct visuals* v) {
    v->A = true;
    v->B = true;
    v->UP = true;
    v->DOWN = true;
    v->LEFT = true;
    v->RIGHT = true;
    v->accel[0] = 512;
    v->accel[1] = 512;
    v->accel[2] = 512;
    v->ir[0] = 756;
    v->ir[1] = 400;
    
    v->hasNunchuk = true;
    v->C = true;
    v->Z = true;
    v->stick[0] = 0;
    v->stick[1] = 255;
	clamp_acc(v);
	clamp_ir(v);
	normalize_joystick(v);
}

void visualize_inputs(const uint8_t *buf, int len) {
	if (closed) return;
	SDL_Event event;
	if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
		return exit_visualizer();
	}
	// change rendered textures depending on changes in input
	//SDL_RenderCopy(gRenderer, texture, NULL, NULL);

	visualize_inputs_console(buf, len);
	if (is_input_report(buf, len)) parse_report(&wm->v, buf, extension_decryption_key);
	//all_on(&wm->v);

	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(gRenderer);

	wm->Draw();
	SDL_RenderPresent(gRenderer);
}

/*int main(void) {
	init_visualizer();
	while (!closed) {
		visualize_inputs(nullptr, 0);
	}
	exit_visualizer();
}*/

