#ifdef _WIN32
#include "SDL.h"
#include "SDL_image.h"
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#include "wm_crypto.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <cstdint>

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
bool closed = false;

// https://lazyfoo.net/tutorials/SDL/40_texture_manipulation/index.php
class Texture {
		SDL_Texture* tex;
	public:
		int width, height;
		Texture(std::string filepath) : tex{nullptr}, width{0}, height{0} {
			//std::cout << "TEX LOAD " << filepath << std::endl;
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
			SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
			SDL_FreeSurface(formatted);
		}

		Texture() : tex{nullptr}, width{0}, height{0} {}

		Texture& operator=(Texture&& t) {
			if (tex) SDL_DestroyTexture(tex);
			tex = t.tex;
			width = t.width;
			height = t.height;
			return *this;
		}
		
		void render(int x, int y, int R = 0xFF, int G = 0xFF, int B = 0xFF) {
			if (x < 0 || y < 0) return;
			//SDL_SetTextureColorMod(tex, R, G, B);
			//SDL_SetRenderDrawColor(gRenderer, R, G, B, 0xFF);
			// int x, iny y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip
			SDL_Rect quad = {x, y, width, height};
			SDL_RenderCopyEx(gRenderer, tex, 0, &quad, 0.0, 0, SDL_FLIP_NONE);
		}
		
		void set_size(int new_width, int new_height) {
			width = new_width;
			height = new_height;
		}

		void set_colour(Uint8 R, Uint8 G, Uint8 B) {
			SDL_SetTextureColorMod(tex, R, G, B);
		}
		
		~Texture() {
			if (tex) SDL_DestroyTexture(tex);
		}
};

class Button {
		Texture* pressed;
		Texture* released;
	public:
		int x, y;
		enum Mode{Fill, Negative, Dark};
		Button(std::string name, Mode m = Mode::Negative) : pressed{nullptr}, released{nullptr}, x{0}, y{0} {
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
		
		void press() { 
			pressed->render(x, y);
		}
		
		void release() {
			released->render(x, y);
		}

		void set_location(int new_x, int new_y) {
			x = new_x;
			y = new_y;
		}

		void set_colour(Uint8 R, Uint8 G, Uint8 B) {
			pressed->set_colour(R, G, B);
			released->set_colour(R, G, B);
		}

		friend std::ifstream& operator>>(std::ifstream& file, Button& B) {
			char line[256]; 
			file.getline(line, 256);
			std::istringstream input = std::istringstream{line};
			int x, y, r, g, b;
			input >> x >> y >> r >> g >> b;
			B.set_location(x, y);
			B.set_colour(r, g, b);
			return file;
		}
};

class DPad {
		Texture outline;
	public:
		Texture *directions[4];
		int x, y;
		int rotation;
		DPad() :
			outline{"d-pad-gate.png"},
			x{0}, y{0},
			rotation{0}
		{
			directions[0] = new Texture{"d-pad-up.png"};
			directions[1] = new Texture{"d-pad-right.png"};
			directions[2] = new Texture{"d-pad-down.png"};
			directions[3] = new Texture{"d-pad-left.png"};
		}

		void set_colour(uint8_t R, uint8_t G, uint8_t B) {
			directions[0]->set_colour(242, 133, 250); // sample numbers for testing
			directions[1]->set_colour(69, 47, 27);
			directions[2]->set_colour(0xFF, 0, 0);
			directions[3]->set_colour(2, 93, 231);
		}

		void render(bool Up, bool Down, bool Left, bool Right) {
			outline.render(x, y);
			//std::cout << "ROT" << rotation << " " << Up << " " << Down << " " << Left << " " << Right << std::endl;
			if (Up) 	directions[(0 + rotation) % 4]->render(x, y);
			if (Right) 	directions[(1 + rotation) % 4]->render(x, y);
			if (Down) 	directions[(2 + rotation) % 4]->render(x, y);
			if (Left) 	directions[(3 + rotation) % 4]->render(x, y);
		}

		friend std::ifstream& operator>>(std::ifstream& file, DPad& d) {
			auto readline = [](std::ifstream& file){
				char line[256]; 
				file.getline(line, 256);
				return std::istringstream{line};
			};
			std::istringstream input = readline(file);
			int x, y, r, g, b;
			input >> x >> y >> r >> g >> b;
			d.x = x;
			d.y = y;
			d.outline.set_colour(r, g, b);
			input = readline(file);
			input >> r;
			d.rotation = r;
			for (int i = 0; i < 4; ++i) {
				input = readline(file);
				input >> r >> g >> b;
				d.directions[i]->set_colour(r, g, b);
			}
			return file;
		}

		~DPad() {
			for (int i = 0; i < 4; ++i) delete directions[i];
		}
};

class IR {
		int x, y, width;
		Uint8 box_r, box_g, box_b;
		Uint8 cursor_r, cursor_g, cursor_b;
	public:
		IR() :
			width{4}
		{
		
		}
		
		void render(uint8_t cursor_x, uint8_t cursor_y) {
			if (x < 0 || y < 0) return;
			
			SDL_Rect r = {x, y, 1024, 1024}; // constant width/height
			SDL_SetRenderDrawColor(gRenderer, box_r, box_g, box_b, 0xFF);
			SDL_RenderDrawRect(gRenderer, &r);
			
			SDL_SetRenderDrawColor(gRenderer, cursor_r, cursor_g, cursor_b, SDL_ALPHA_OPAQUE);
			//SDL_RenderDrawPoint(gRenderer, cursor_x, cursor_y);
			for (int i = 0; i < width; ++i) {
				for (int j = 0; j < width; ++j) {
					SDL_RenderDrawPoint(gRenderer, x + cursor_x + i, y + cursor_y + j);
				}
			}
						
		}

		friend std::ifstream& operator>>(std::ifstream& file, IR& ir) {
			auto readline = [](std::ifstream& file){
				char line[256]; 
				file.getline(line, 256);
				return std::istringstream{line};
			};
			std::istringstream input = readline(file);
			input >> ir.x >> ir.y;
			input >> ir.box_r >> ir.box_g >> ir.box_b;
			input = readline(file);
			input >> ir.cursor_r >> ir.cursor_g >> ir.cursor_b;
			return file;
		}
		
};

class Joystick {
		Texture gate;
		bool enable_bounding_box;
		Uint8 stick_r, stick_g, stick_b;
	public:
		int x, y;
		enum Style {Line, Stick}; // ToDo: option to not use a line

		Joystick() :
			gate{"joystick-gate.png"},
			enable_bounding_box{true}
		{

		}

		void render(uint8_t jx, uint8_t jy) {
			if (x < 0 || y < 0) return;
			if (enable_bounding_box) {
				SDL_Rect r = {x, y, gate.width, gate.height};
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0x60);
				SDL_RenderDrawRect(gRenderer, &r);
			}
			gate.render(x, y);
			DrawStick((jx-127)/2, (128-jy)/2, 3);
		}

		friend std::ifstream& operator>>(std::ifstream& file, Joystick& J) {
			auto readline = [](std::ifstream& file){
				char line[256]; 
				file.getline(line, 256);
				return std::istringstream{line};
			};
			std::istringstream input = readline(file);
			int x, y, r, g, b;
			input >> x >> y >> r >> g >> b;
			J.x = x;
			J.y = y;
			J.gate.set_colour(r, g, b);
			input = readline(file);
			input >> r >> g >> b;
			J.stick_r = r;
			J.stick_g = g;
			J.stick_b = b;
			input = readline(file);
			input >> x;
			J.enable_bounding_box = x > 0;
			return file;
		}

	private:
		void DrawStick(int jx, int jy, int width) { // expects numbers from -127 to 128
			if (width < 1) return;
			int cx = x + gate.width/2; // center
			int cy = y + gate.height/2;
			SDL_SetRenderDrawColor(gRenderer, stick_r, stick_g, stick_b, 0xFF);
			SDL_RenderDrawLine(gRenderer,cx,cy,cx+jx,cy+jy);
			for (int i = 1; i < width; ++i) {
				// To speed up: determine quadrant of (jx,jy) and only draw lines
				// along a diagonal in the opposite quadrant
				// ToDo: keep within border
				for (int j = i; j >= 0; --j) { // draw diagonals around (cx,cy)
					SDL_RenderDrawLine(gRenderer,cx+j,cy-i+j,cx+jx+j,cy+jy-i+j); // top right
					SDL_RenderDrawLine(gRenderer,cx-j,cy+i-j,cx+jx-j,cy+jy+i-j); // bottom left
					SDL_RenderDrawLine(gRenderer,cx+j,cy+i-j,cx+jx+j,cy+jy+i-j); // bottom right
					SDL_RenderDrawLine(gRenderer,cx-j,cy-i+j,cx+jx-j,cy+jy-i+j); // top left
				}
			}
		}
};

struct visuals {
    bool A;
    bool B;
    bool UP;
    bool DOWN;
    bool LEFT;
    bool RIGHT;
    bool ONE;
    bool TWO;
    bool PLUS;
    bool MINUS;
    bool HOME;
    int accel[3];
    int ir[2];
    
    bool hasNunchuk;
    bool C;
    bool Z;
    int stick[2];
    visuals() {}
};

struct Layout {
	visuals v;
	const uint8_t type;
	int background[3];
	Layout(const uint8_t type) : v{}, type{type} {}
	virtual void Draw() {}
};

class WiimoteLayout : public Layout {
	Button A, B, One, Two, Plus, Minus, Home;
	DPad d;
	IR ir;
	public:
		WiimoteLayout(uint8_t type) :
			Layout{type},
			A{"a", Button::Mode::Negative},
			B{"b", Button::Mode::Negative},
			One{"1", Button::Mode::Fill}, // TODO: add these textures
			Two{"2", Button::Mode::Fill}, // try to abstract it all more (this is repeated in nunchuk layout)
			Plus{"+", Button::Mode::Fill},
			Minus{"-", Button::Mode::Fill},
			Home{"home", Button::Mode::Fill},
			d{},
			ir{}
		{
			std::cout << "New Layout: Wiimote" << std::endl;
			std::ifstream config{"./config/wiimote.layout"};
			if (!config.is_open()) {
				std::cerr << "Could not load wiimote.layout" << std::endl;
				return;
			}

			char line[256]; 
			config.getline(line, 256);
			std::istringstream input = std::istringstream{line};
			input >> background[0] >> background[1] >> background[2];
			config >> A >> B >> One >> Two >> Plus >> Minus >> Home >> d >> ir;
			config.close();
		}

		void Draw() override {
			v.A ? A.press() : A.release();
			v.B ? B.press() : B.release();
			v.ONE ? One.press() : One.release();
			v.TWO ? Two.press() : Two.release();
			v.PLUS ? Plus.press() : Plus.release();
			v.MINUS ? Minus.press() : Minus.release();
			v.HOME ? Home.press() : Home.release();
			d.render(v.UP, v.DOWN, v.LEFT, v.RIGHT);
			ir.render(v.ir[0], v.ir[1]);
		}
};

class NunchukLayout : public Layout {
		Button A, B, One, Two, Plus, Minus, Home, C, Z;
		DPad d;
		Joystick j;
		IR ir;
	public:

		NunchukLayout() :
			Layout{0x37},
			A{"a", Button::Mode::Negative},
			B{"b", Button::Mode::Negative},
			One{"1", Button::Mode::Fill},
			Two{"2", Button::Mode::Fill},
			Plus{"+", Button::Mode::Fill},
			Minus{"-", Button::Mode::Fill},
			Home{"home", Button::Mode::Fill},
			C{"c", Button::Mode::Negative},
			Z{"z", Button::Mode::Negative},
			d{},
			j{},
			ir{}
		{
			std::cout << "New Layout: Wiimote+Nunchuk" << std::endl;
			std::ifstream config{"./config/nunchuk.layout"};
			if (!config.is_open()) {
				std::cerr << "Could not load nunchuk.layout" << std::endl;
				return;
			}

			char line[256]; 
			config.getline(line, 256);
			std::istringstream input = std::istringstream{line};
			input >> background[0] >> background[1] >> background[2];
			config >> A >> B >> One >> Two >> Plus >> Minus >> Home >> d;
			config >> C >> Z >> j >> ir;
			config.close();

			// P1 light blue: 100,255,255 
			// P2 yellow: 255,220,0
		}
		
		void Draw() override {
			if (v.A) { A.press(); } else { A.release(); }
			if (v.B) { B.press(); } else { B.release(); }
			if (v.C) { C.press(); } else { C.release(); }
			if (v.Z) { Z.press(); } else { Z.release(); }
			j.render(v.stick[0], v.stick[1]);
			d.render(v.UP, v.DOWN, v.LEFT, v.RIGHT);
			ir.render(v.ir[0], v.ir[1]);
		}
};

void reload_layout(struct Layout **L, uint8_t type) {
	struct Layout *L2 = nullptr;
	if (type == 0x30 || type == 0x31 || type == 0x33) {
		L2 = new WiimoteLayout(type);
	} else if (type == 0x37) {
		L2 = new NunchukLayout();
	} else {
		//std:: cerr << "ERROR: Unsupported Report Type " << (int)type << std::endl;
		return;
	}
	delete *L;
	*L = L2;
}

void clamp_ir(struct visuals *v) {
    v->ir[0] /= 4;
    v->ir[1] /= 4;
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

// assumes it's given an expected report
// key length = 16
void parse_report(struct visuals *v, const uint8_t *buf, struct ext_crypto_state* key) {
    //for (int i = 0; i < 16; ++i) printf("%02X ", key[i]);
    v->LEFT = buf[2] & 0x01;
    v->RIGHT = buf[2] & 0x02;
    v->DOWN = buf[2] & 0x04;
    v->UP = buf[2] & 0x08;
    v->B = buf[3] & 0x04;
    v->A = buf[3] & 0x08;
	v->ONE = buf[3] & 0x02;
	v->TWO = buf[3] & 0x01;
	v->PLUS = buf[2] & 0x10;
	v->MINUS = buf[3] & 0x10;
	v->HOME = buf[3] & 0x80;
    if (buf[1] != 0x30) { // other reporting modes dont have this as well...
		v->accel[0] = (buf[4] << 2) + ((buf[2] & 0x60) >> 5);
    	v->accel[1] = (buf[5] << 2) + ((buf[3] & 0x20) >> 4);
    	v->accel[2] = (buf[6] << 2) + ((buf[3] & 0x40) >> 5);
	}
    v->ir[0] = 0;
    v->ir[1] = 0;
    if (buf[1] == 0x33) {
        v->hasNunchuk = false;
        for (int i = 0; i < 4; i++) { // 12 byte IR // this is untested (probably broken)
            v->ir[0] += buf[7 + i * 2] + ((buf[9 + i * 2] & 0x30) << 4);
            v->ir[1] += buf[8 + i * 2] + ((buf[9 + i * 2] & 0xC0) << 2);
        }
    } else if (buf[1] == 0x37) {
        v->hasNunchuk = true;
        for (int i = 7; i <=12; i += 5) { // 10 byte IR, 2 iterations
            v->ir[0] += buf[i + 0] + ((buf[i + 2] & 0x30) << 4);
            v->ir[1] += buf[i + 1] + ((buf[i + 2] & 0xC0) << 2);
            v->ir[0] += buf[i + 3] + ((buf[i + 2] & 0x03) << 8);
            v->ir[1] += buf[i + 4] + ((buf[i + 2] & 0x0C) << 6);
        }
        // ((data[i] ^ key[8 + i]) + key[i]) % 0x100
        uint8_t ext_data[6] = {0};
        memcpy(ext_data, buf+17, sizeof(uint8_t)*6);
        ext_decrypt_bytes(key, ext_data, 0, 6);
        v->stick[0] = ext_data[0];//((buf[17] ^ key[8]) + key[0]) % 0x100;
        v->stick[1] = ext_data[1];//((buf[18] ^ key[9]) + key[1]) % 0x100;
        //printf("<- key\nbuf[17..22] = %02X %02X %02X %02X %02X %02X\n", buf[17], buf[18], buf[19], buf[20], buf[21], buf[22]);
        uint8_t decrypted_final = ext_data[5];//((buf[22] ^ key[13]) + key[5]) % 0x100;
        //printf("%02X ^ %02X + %02X\n", buf[22], key[13], key[5]);
        //printf("DECRYPTED BYTES: %02X %02X %02X\n", v->stick[0], v->stick[1], decrypted_final);
        v->Z = (decrypted_final & 0x1) == 0;
        v->C = (decrypted_final & 0x2) == 0;
    }
    clamp_ir(v);
    clamp_acc(v);
}

// returns true if the report is either nunchuk data or no extension w/IR
bool is_input_report(const uint8_t * buf, int len) {
    return (
		buf != nullptr &&
		len > 0 &&
        buf[0] == 0xA1 &&
        (
            (len == 0x17 && buf[1] == 0x37) ||
            (len == 0x13 && buf[1] == 0x33) ||
			(buf[1] == 0x30 || buf[1] == 0x31 || buf[1] == 0x33) // wiimote types
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
struct ext_crypto_state decrypt_state = {{0}, {0}};
enum KeyState {Uninitialized, EncryptionEnabled, Block1, Block2, Complete};
int state = Uninitialized;

// https://wiibrew.org/wiki/Wiimote/Protocol#Extension_Controllers
void store_extension_key(const uint8_t *buf, int len) { // output report (from wii) buf[0]=a2
	if (len < 1 + 6 + 16) return; // a2 16 MM FF FF FF SS + key
	//if (state == Uninitialized) printf("Out Report: %02X %02X %02X %02X %02X %02X...\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	if (buf[1] != 0x16) return; // write memory register
	if (buf[2] != 0x04) return; // enable write data
	if (buf[3] != 0xA4 || buf[4] != 0x00) return; // register choice
	if (state == Uninitialized) {
		if (buf[5] == 0xF0 && buf[6] == 0x01 && buf[7] == 0xAA) {
			state = EncryptionEnabled;
			//printf("ENABLE ENCRYPTION\n");
		} else if (buf[5] == 0x40 && buf[6] == 16) { // idk if the wii ever does this?
			memcpy(extension_decryption_key, buf+7, sizeof(uint8_t)*16);
			//for (int i = 0; i < 16; ++i) printf("%02X ", extension_decryption_key[i]);
			//printf("<- KEY COMPLETE\n");
			ext_generate_tables(&decrypt_state, buf+7);
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
			//printf("KEY BLOCK1\n");
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
			//printf("KEY BLOCK2\n");
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
			//for (int i = 0; i < 16; ++i) printf("%02X ", extension_decryption_key[i]);
			//printf("<- KEY COMPLETE\n");
			ext_generate_tables(&decrypt_state, extension_decryption_key);
		}
	}
}

//int extkeycounter = 0;
void visualize_inputs_console(const uint8_t *buf, int len) {//, const unit8_t *key) {
    if (!is_input_report(buf, len)) return;
    struct visuals v;
    parse_report(&v, buf, &decrypt_state);
    for (int i = 0; i < len; ++i) printf("%02X", buf[i]);
    printf(" ");
    /*printf("=> ");
    if (++extkeycounter == 50) {
    	extkeycounter = 0;
    	printf("( ");
    	for (int i = 0; i < 16; ++i) printf("%02X ", extension_decryption_key[i]);
    	printf(") ");
    }*/
    print_inputs(&v);
}

Layout *L = nullptr;

int init_visualizer(void) {
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(550, 250, 0, &gWindow, &gRenderer);
	SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_ADD);
	IMG_Init(IMG_INIT_PNG);
	L = new Layout(0x0);
	return 1;
}

void exit_visualizer() {
	if (closed) return;
	closed = 1;
	delete L;
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
    v->stick[0] = 255;
    v->stick[1] = 255;
    clamp_acc(v);
    clamp_ir(v);
}

void visualize_inputs(const uint8_t *buf, int len) {
	if (closed || len < 2) return;
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return exit_visualizer();
		} else if (event.type = SDL_KEYDOWN && event.key.keysym.mod & KMOD_CTRL && event.key.keysym.sym == SDLK_r) {
			reload_layout(&L, buf[1]);
		}
	}
	if (buf[1] != L->type) reload_layout(&L, buf[1]);
	//visualize_inputs_console(buf, len);
	if (is_input_report(buf, len)) parse_report(&L->v, buf, &decrypt_state);
#ifdef VISTEST
	all_on(&L->v);
#endif
	SDL_SetRenderDrawColor(gRenderer, L->background[0], L->background[1], L->background[2], 0xFF);
	SDL_RenderClear(gRenderer);
	L->Draw();
	SDL_RenderPresent(gRenderer);
}

#ifdef VISTEST
int main(void) {
	init_visualizer();
	const uint8_t sample_buf[8] = {0xA1, 0x31, 0x04, 0x02, 0x80, 0x80, 0x9A, 0x07};
	while (!closed) {
		visualize_inputs(sample_buf, 8);
	}
	exit_visualizer();
}
#endif

