
#include "img_in.h"

#include "libraw/libraw.h"

#define RAWCTX()	LIBRAWDec *ctx = (LIBRAWDec *) ((IMGDec *)ifcg->privateStack)->opaque
#define HANDLE_ERRORS() if (ret) fprintf(stderr, "libraw  %s\n", libraw_strerror(ret))

typedef struct
{
	u16 ES_ID;
	u32 BPP, width, height, out_size, pixel_format;
	u32 state;
	libraw_data_t *iprc;
	libraw_processed_image_t *image;
} LIBRAWDec;


static GF_Err LIBRAW_AttachStream(GF_BaseDecoder *ifcg, GF_ESD *esd)
{
	RAWCTX();

	if (ctx->ES_ID && ctx->ES_ID != esd->ESID) return GF_NOT_SUPPORTED;
	ctx->ES_ID = esd->ESID;
	ctx->BPP = 3;
	ctx->iprc = libraw_init(0);
	return GF_OK;
}
static GF_Err LIBRAW_DetachStream(GF_BaseDecoder *ifcg, u16 ES_ID)
{
	RAWCTX();
	if (ctx->ES_ID != ES_ID) return GF_BAD_PARAM;
	ctx->ES_ID = ES_ID;
	if (ctx->image) libraw_dcraw_clear_mem(ctx->image);
	libraw_close(ctx->iprc);
	return GF_OK;
}
static GF_Err LIBRAW_GetCapabilities(GF_BaseDecoder *ifcg, GF_CodecCapability *capability)
{
	RAWCTX();
	switch (capability->CapCode) {
	case GF_CODEC_WIDTH:
		capability->cap.valueInt = ctx->width;
		break;
	case GF_CODEC_HEIGHT:
		capability->cap.valueInt = ctx->height;
		break;
	case GF_CODEC_STRIDE:
		capability->cap.valueInt = ctx->width * ctx->BPP;
		break;
	case GF_CODEC_FPS:
		capability->cap.valueFloat = 0;
		break;
	case GF_CODEC_PIXEL_FORMAT:
		capability->cap.valueInt = ctx->pixel_format;
		break;
	case GF_CODEC_OUTPUT_SIZE:
		capability->cap.valueInt = ctx->out_size;
		break;
	case GF_CODEC_BUFFER_MIN:
		capability->cap.valueInt = 0;
		break;
	case GF_CODEC_BUFFER_MAX:
		capability->cap.valueInt = IMG_CM_SIZE;
		break;
	case GF_CODEC_PADDING_BYTES:
		capability->cap.valueInt = 4;
		break;
	case GF_CODEC_PAR:
		capability->cap.valueInt = 0;
		break;
	case GF_CODEC_REORDER:
		capability->cap.valueInt = 0;
		break;
	default:
		capability->cap.valueInt = 0;
		return GF_NOT_SUPPORTED;
	}
	return GF_OK;
}
static GF_Err LIBRAW_SetCapabilities(GF_BaseDecoder *ifcg, GF_CodecCapability capability)
{
	/*return unsupported to avoid confusion by the player (like color space changing ...) */
	return GF_NOT_SUPPORTED;
}

static const char *LIBRAW_GetCodecName(GF_BaseDecoder *dec)
{
	return "LibRaw";
}

static GF_Err LIBRAW_ProcessData(GF_MediaDecoder *ifcg,
	char *inBuffer, u32 inBufferLength,
	u16 ES_ID, u32 *CTS,
	char *outBuffer, u32 *outBufferLength,
	u8 PaddingBits, u32 mmlevel)
{
	RAWCTX();

	if (!ctx->image){
		libraw_processed_image_t * out;

		int ret = libraw_open_buffer(ctx->iprc, inBuffer, inBufferLength);
		HANDLE_ERRORS();

		ret = libraw_unpack(ctx->iprc);
		HANDLE_ERRORS();

		ret = libraw_dcraw_process(ctx->iprc);
		HANDLE_ERRORS();

		out = libraw_dcraw_make_mem_image(ctx->iprc, &ret);
		HANDLE_ERRORS();

		ctx->image = out;
		ctx->width = out->width;
		ctx->height = out->height;
		ctx->BPP = out->colors;
		ctx->pixel_format = GF_PIXEL_RGB_24;
	}
	

	if (*outBufferLength < ctx->image->data_size) {
		ctx->out_size = ctx->image->data_size;
		*outBufferLength = ctx->out_size;
		return GF_BUFFER_TOO_SMALL;
	}

	memcpy(outBuffer, ctx->image->data, ctx->image->data_size);
	return GF_OK;
}

Bool NewRAWDec(GF_BaseDecoder *ifcd)
{
	IMGDec *wrap = (IMGDec *)ifcd->privateStack;
	LIBRAWDec *dec = (LIBRAWDec *)gf_malloc(sizeof(LIBRAWDec));
	memset(dec, 0, sizeof(LIBRAWDec));
	wrap->opaque = dec;
	wrap->type = DEC_DNG;

	/*setup our own interface*/
	ifcd->AttachStream = LIBRAW_AttachStream;
	ifcd->DetachStream = LIBRAW_DetachStream;
	ifcd->GetCapabilities = LIBRAW_GetCapabilities;
	ifcd->SetCapabilities = LIBRAW_SetCapabilities;
	ifcd->GetName = LIBRAW_GetCodecName;
	((GF_MediaDecoder *)ifcd)->ProcessData = LIBRAW_ProcessData;
	return GF_TRUE;
}

void DeleteRAWDec(GF_BaseDecoder *ifcg)
{
	RAWCTX();
	gf_free(ctx);
}
