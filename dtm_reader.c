#include "dtm_reader.h"

static FILE *dtm = NULL;
static uint8_t key[16] = {0};

// returns 0 on success
static int load_dtm(void)
{
  // load extension decryption key
  FILE *keyf = fopen(DEFAULT_KEY, "r");
  if (keyf == NULL)
  {
    fprintf(stderr, "Error opening dtm encryption key: '%s'\n", DEFAULT_KEY);
    return -1;
  }
  int data;
  for (int i = 0; i < 16; ++i)
  {
    if (fscanf(keyf, "%x", &data) != 1) {
      fprintf(stderr, "Error reading dtm encryption key. Ensure '%s' has 16 space separated hexadecimal bytes.\n", DEFAULT_KEY);
      return -1;
    }
    key[i] = (uint8_t)data;
  }
  fclose(keyf);

  // load dtm
  dtm = fopen(DEFAULT_TAS, "rb");
  if (dtm == NULL)
  {
    fprintf(stderr, "Error opening dtm\n");
    return -1;
  }
  fseek(dtm, 0x100, SEEK_SET); // skip header
  return 0;
}

// returns the length of buf
int next_dtm_report(struct wiimote_state *state, uint8_t *buf)
{
  if (dtm == NULL && load_dtm())
  {
    return 0;
  }

  uint8_t len;
  if (
    fread(&len, sizeof(len), 1, dtm) == 0 || // read length
    fread(buf, sizeof(uint8_t) * len, len, dtm) == 0 // read encrypted report
  ) {
    fclose(dtm);
    dtm = NULL;
    return 0;
  }

  // re-encrypt extension data
  int offset = 0, ext_len = 0;
  switch (buf[1])
  {
    case 0x32:
      offset = 4, ext_len = 8;
      break;
    case 0x34:
      offset = 4, ext_len = 19;
      break;
    case 0x35:
      offset = 7, ext_len = 16;
      break;
    case 0x36:
      offset = 14, ext_len = 9;
      break;
    case 0x37:
      offset = 17, ext_len = 6;
      break;
    case 0x3D:
      offset = 2, ext_len = 21;
      break;
  }

  if (ext_len)
  {
    ext_decrypt_bytes((struct ext_crypto_state *)&key, buf + offset, 0, ext_len);
  }
  if (state->sys.extension_encrypted)
  {
    ext_encrypt_bytes(&state->sys.extension_crypto_state, buf + offset, 0, ext_len);
  }
  return (int)len;
}
