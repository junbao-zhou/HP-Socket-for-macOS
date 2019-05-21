//=====================================================================
//
// KCP - A Better ARQ Protocol Implementation
// skywind3000 (at) gmail.com, 2010-2011
//  
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
//=====================================================================
#include "ikcp.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wconversion"
	#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
/*
 * RTO:超时重传时间
 * RTT:请求应答时间
 */

//=====================================================================
// KCP BASIC
//=====================================================================
const IUINT32 IKCP_RTO_NDL = 30;		// no delay min rto 无迟延最小超时重传时间ms
const IUINT32 IKCP_RTO_MIN = 100;		// normal min rto 正常最小超时重传时间ms
const IUINT32 IKCP_RTO_DEF = 200;		// 超时重传时间定义
const IUINT32 IKCP_RTO_MAX = 60000;		// 最大超时重传时间定义
const IUINT32 IKCP_CMD_PUSH = 81;		// cmd: push data 具体的数据包
const IUINT32 IKCP_CMD_ACK  = 82;		// cmd: ack 通知包，通知对端收到那些数据包
const IUINT32 IKCP_CMD_WASK = 83;		// cmd: window probe (ask) 探测包，探测对端窗口大小
const IUINT32 IKCP_CMD_WINS = 84;		// cmd: window size (tell) 告诉对端窗口大小
const IUINT32 IKCP_ASK_SEND = 1;		// need to send IKCP_CMD_WASK 
const IUINT32 IKCP_ASK_TELL = 2;		// need to send IKCP_CMD_WINS
const IUINT32 IKCP_WND_SND = 32;		// 发送窗口，以数据包为单位
const IUINT32 IKCP_WND_RCV = 128;       // must >= max fragment size
const IUINT32 IKCP_MTU_DEF = 1400; 		// MTU(最大传输单元)定义
const IUINT32 IKCP_ACK_FAST	= 3;		// 用于快速重传，Ack被跳过3次后重传
const IUINT32 IKCP_INTERVAL	= 100;		// 调度间隔 
const IUINT32 IKCP_OVERHEAD = 24;		// 固定头部长度
const IUINT32 IKCP_DEADLINK = 20;		// 最大重传次数
const IUINT32 IKCP_THRESH_INIT = 2;		// 拥塞窗口阈值，以包为单位
const IUINT32 IKCP_THRESH_MIN = 2;		// 最小拥塞窗口阈值，以包为单位
const IUINT32 IKCP_PROBE_INIT = 7000;		// 7 secs to probe window size 探测窗口间隔
const IUINT32 IKCP_PROBE_LIMIT = 120000;	// up to 120 secs to probe window 最大探测间隔


//---------------------------------------------------------------------
// encode / decode
//---------------------------------------------------------------------

/* encode 8 bits unsigned int */
static inline char *ikcp_encode8u(char *p, unsigned char c)
{
	*(unsigned char*)p++ = c;
	return p;
}

/* decode 8 bits unsigned int */
static inline const char *ikcp_decode8u(const char *p, unsigned char *c)
{
	*c = *(unsigned char*)p++;
	return p;
}

/* encode 16 bits unsigned int (lsb) */
static inline char *ikcp_encode16u(char *p, unsigned short w)
{
#if IWORDS_BIG_ENDIAN
	*(unsigned char*)(p + 0) = (w & 255);
	*(unsigned char*)(p + 1) = (w >> 8);
#else
	*(unsigned short*)(p) = w;
#endif
	p += 2;
	return p;
}

/* decode 16 bits unsigned int (lsb) */
static inline const char *ikcp_decode16u(const char *p, unsigned short *w)
{
#if IWORDS_BIG_ENDIAN
	*w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
	*w = *(const unsigned short*)p;
#endif
	p += 2;
	return p;
}

/* encode 32 bits unsigned int (lsb) */
static inline char *ikcp_encode32u(char *p, IUINT32 l)
{
#if IWORDS_BIG_ENDIAN
	*(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
	*(IUINT32*)p = l;
#endif
	p += 4;
	return p;
}

/* decode 32 bits unsigned int (lsb) */
static inline const char *ikcp_decode32u(const char *p, IUINT32 *l)
{
#if IWORDS_BIG_ENDIAN
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	*l = *(const IUINT32*)p;
#endif
	p += 4;
	return p;
}

/**
 * 取两数最小
 */
static inline IUINT32 _imin_(IUINT32 a, IUINT32 b) {
	return a <= b ? a : b;
}

/**
 * 取两数最大
 */
static inline IUINT32 _imax_(IUINT32 a, IUINT32 b) {
	return a >= b ? a : b;
}

/**
 * 限定边界
 * return: lower <= result <= upper
 */
static inline IUINT32 _ibound_(IUINT32 lower, IUINT32 middle, IUINT32 upper) 
{
	return _imin_(_imax_(lower, middle), upper);
}

/**
 * 取差值
 */
static inline long _itimediff(IUINT32 later, IUINT32 earlier) 
{
	return ((IINT32)(later - earlier));
}

//---------------------------------------------------------------------
// manage segment
//---------------------------------------------------------------------
typedef struct IKCPSEG IKCPSEG;

static void* (*ikcp_malloc_hook)(size_t) = NULL;
static void (*ikcp_free_hook)(void *) = NULL;

// internal malloc
static void* ikcp_malloc(size_t size) {
	if (ikcp_malloc_hook) 
		return ikcp_malloc_hook(size);
	return malloc(size);
}

// internal free
static void ikcp_free(void *ptr) {
	if (ikcp_free_hook) {
		ikcp_free_hook(ptr);
	}	else {
		free(ptr);
	}
}

// redefine allocator
void ikcp_allocator(void* (*new_malloc)(size_t), void (*new_free)(void*))
{
	ikcp_malloc_hook = new_malloc;
	ikcp_free_hook = new_free;
}

// allocate a new kcp segment
static IKCPSEG* ikcp_segment_new(ikcpcb *kcp, int size)
{
	return (IKCPSEG*)ikcp_malloc(sizeof(IKCPSEG) + size);
}

// delete a segment
static void ikcp_segment_delete(ikcpcb *kcp, IKCPSEG *seg)
{
	ikcp_free(seg);
}

// write log
void ikcp_log(ikcpcb *kcp, int mask, const char *fmt, ...)
{
	char buffer[1024];
	va_list argptr;
	if ((mask & kcp->logmask) == 0 || kcp->writelog == 0) return;
	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);
	kcp->writelog(buffer, kcp, kcp->user);
}

// check log mask
static int ikcp_canlog(const ikcpcb *kcp, int mask)
{
	if ((mask & kcp->logmask) == 0 || kcp->writelog == NULL) return 0;
	return 1;
}

/*! 调用输出回调进行发送分片 */
// output segment
static int ikcp_output(ikcpcb *kcp, const void *data, int size)
{
	assert(kcp);
	assert(kcp->output);
	if (ikcp_canlog(kcp, IKCP_LOG_OUTPUT)) {
		ikcp_log(kcp, IKCP_LOG_OUTPUT, "[RO] %ld bytes", (long)size);
	}
	if (size == 0) return 0;
	return kcp->output((const char*)data, size, kcp, kcp->user);
}

// output queue
void ikcp_qprint(const char *name, const struct IQUEUEHEAD *head)
{
#if 0
	const struct IQUEUEHEAD *p;
	printf("<%s>: [", name);
	for (p = head->next; p != head; p = p->next) {
		const IKCPSEG *seg = iqueue_entry(p, const IKCPSEG, node);
		printf("(%lu %d)", (unsigned long)seg->sn, (int)(seg->ts % 10000));
		if (p->next != head) printf(",");
	}
	printf("]\n");
#endif
}


//---------------------------------------------------------------------
// create a new kcpcb
//---------------------------------------------------------------------
ikcpcb* ikcp_create(IUINT32 conv, void *user)
{
	ikcpcb *kcp = (ikcpcb*)ikcp_malloc(sizeof(struct IKCPCB));
	if (kcp == NULL) return NULL;
	kcp->conv = conv;
	kcp->user = user;
	kcp->snd_una = 0;
	kcp->snd_nxt = 0;
	kcp->rcv_nxt = 0;
	kcp->ts_recent = 0;
	kcp->ts_lastack = 0;
	kcp->ts_probe = 0;
	kcp->probe_wait = 0;
	kcp->snd_wnd = IKCP_WND_SND;
	kcp->rcv_wnd = IKCP_WND_RCV;
	kcp->rmt_wnd = IKCP_WND_RCV;
	kcp->cwnd = 0;
	kcp->incr = 0;
	kcp->probe = 0;
	kcp->mtu = IKCP_MTU_DEF;
	kcp->mss = kcp->mtu - IKCP_OVERHEAD; //最大分片大小
	kcp->stream = 0;

	kcp->buffer = (char*)ikcp_malloc((kcp->mtu + IKCP_OVERHEAD) * 3);
	if (kcp->buffer == NULL) {
		ikcp_free(kcp);
		return NULL;
	}

	/*! 初始化使得前后指向均指向自身 */
	iqueue_init(&kcp->snd_queue);
	iqueue_init(&kcp->rcv_queue);
	iqueue_init(&kcp->snd_buf);
	iqueue_init(&kcp->rcv_buf);
	kcp->nrcv_buf = 0;
	kcp->nsnd_buf = 0;
	kcp->nrcv_que = 0;
	kcp->nsnd_que = 0;
	kcp->state = 0;
	kcp->acklist = NULL;
	kcp->ackblock = 0;
	kcp->ackcount = 0;
	kcp->rx_srtt = 0;
	kcp->rx_rttval = 0;
	kcp->rx_rto = IKCP_RTO_DEF;
	kcp->rx_minrto = IKCP_RTO_MIN;
	kcp->current = 0;
	kcp->interval = IKCP_INTERVAL;
	kcp->ts_flush = IKCP_INTERVAL;
	kcp->nodelay = 0;
	kcp->updated = 0;
	kcp->logmask = 0;
	kcp->ssthresh = IKCP_THRESH_INIT;
	kcp->fastresend = 0;
	kcp->nocwnd = 0;
	kcp->xmit = 0;
	kcp->dead_link = IKCP_DEADLINK;
	kcp->output = NULL;
	kcp->writelog = NULL;

	return kcp;
}


//---------------------------------------------------------------------
// release a new kcpcb
//---------------------------------------------------------------------
void ikcp_release(ikcpcb *kcp)
{
	assert(kcp);
	if (kcp) {
		IKCPSEG *seg;
		//发送buffer
		while (!iqueue_is_empty(&kcp->snd_buf)) {
			///从发送buffer链表中取出下一个并释放
			seg = iqueue_entry(kcp->snd_buf.next, IKCPSEG, node);
			//清除前后指向信息
			iqueue_del(&seg->node);
			//具体释放
			ikcp_segment_delete(kcp, seg);
		}
		//接收buff
		while (!iqueue_is_empty(&kcp->rcv_buf)) {
			seg = iqueue_entry(kcp->rcv_buf.next, IKCPSEG, node);
			iqueue_del(&seg->node);
			ikcp_segment_delete(kcp, seg);
		}
		//发送队列
		while (!iqueue_is_empty(&kcp->snd_queue)) {
			seg = iqueue_entry(kcp->snd_queue.next, IKCPSEG, node);
			iqueue_del(&seg->node);
			ikcp_segment_delete(kcp, seg);
		}
		//接收队列
		while (!iqueue_is_empty(&kcp->rcv_queue)) {
			seg = iqueue_entry(kcp->rcv_queue.next, IKCPSEG, node);
			iqueue_del(&seg->node);
			ikcp_segment_delete(kcp, seg);
		}
		if (kcp->buffer) {
			ikcp_free(kcp->buffer);
		}
		if (kcp->acklist) {
			ikcp_free(kcp->acklist);
		}

		kcp->nrcv_buf = 0;
		kcp->nsnd_buf = 0;
		kcp->nrcv_que = 0;
		kcp->nsnd_que = 0;
		kcp->ackcount = 0;
		kcp->buffer = NULL;
		kcp->acklist = NULL;
		ikcp_free(kcp);
	}
}


/**
 * 设置输出回调，由KCP内部机制调用
 */
//---------------------------------------------------------------------
// set output callback, which will be invoked by kcp
//---------------------------------------------------------------------
void ikcp_setoutput(ikcpcb *kcp, int (*output)(const char *buf, int len,
	ikcpcb *kcp, void *user))
{
	kcp->output = output;
}


/**
 * 该函数主要做了从"接收队列"copy数据到外部buffer中
 * 且从"接收缓存"中移动分片数据到"接收队列"中
 */
//---------------------------------------------------------------------
// user/upper level recv: returns size, returns below zero for EAGAIN
//---------------------------------------------------------------------
int ikcp_recv(ikcpcb *kcp, char *buffer, int len)
{
	struct IQUEUEHEAD *p;
	int ispeek = (len < 0)? 1 : 0;
	int peeksize;
	int recover = 0;
	IKCPSEG *seg;
	assert(kcp);

	//接收队列不为空
	/*! next不指向自己 */
	if (iqueue_is_empty(&kcp->rcv_queue))
		return -1;

	
	if (len < 0) len = -len;

	/*! 窥探kcp中接收数据的长度 */
	peeksize = ikcp_peeksize(kcp);

	/*! kcp的接收队列有点问题 */
	if (peeksize < 0) 
		return -2;

	/*! 准备的buffer长度len不够装的 */
	if (peeksize > len) 
		return -3;

	/*! 接收队列中的数量大于接收窗口的数量时恢复可接收 */
	if (kcp->nrcv_que >= kcp->rcv_wnd)
		recover = 1;

	/*! 将数据队列中copy到外部缓冲池中 */
	// merge fragment
	for (len = 0, p = kcp->rcv_queue.next; p != &kcp->rcv_queue; ) {
		int fragment;
		seg = iqueue_entry(p, IKCPSEG, node);
		p = p->next;
		
		/*! 将分片数据copy到buffer中，buffer会不会溢出呢？ */
		if (buffer) {
			memcpy(buffer, seg->data, seg->len);
			buffer += seg->len;
		}

		len += seg->len;
		fragment = seg->frg;

		if (ikcp_canlog(kcp, IKCP_LOG_RECV)) {
			ikcp_log(kcp, IKCP_LOG_RECV, "recv sn=%lu", seg->sn);
		}

		/*! 能过buffer的长度，进行判断是否将此分片移除 */
		/*! 因为上面将分片中的数据copy进了buffer中 */
		if (ispeek == 0) {
			iqueue_del(&seg->node);
			ikcp_segment_delete(kcp, seg);
			kcp->nrcv_que--;
		}

		/*! 如果分片的编号为0则表明该"数据包"已经被copy完毕 */
		if (fragment == 0) 
			break;
	}

	assert(len == peeksize);

	/*! 从接收buffer中copy数据到接收队列中 */
	// move available data from rcv_buf -> rcv_queue
	while (! iqueue_is_empty(&kcp->rcv_buf)) {
		IKCPSEG *seg = iqueue_entry(kcp->rcv_buf.next, IKCPSEG, node);
		/*! 判断当前接收buf的段编号是否为需要的编号 */
		/*! 且判断接收队列编号是否小于接收窗口 */
		if (seg->sn == kcp->rcv_nxt && kcp->nrcv_que < kcp->rcv_wnd) {
			//删除该结点的前后指向
			iqueue_del(&seg->node);
			//减少接收buffer的长度
			kcp->nrcv_buf--;
			//尾部追加该结点到接收队列中
			iqueue_add_tail(&seg->node, &kcp->rcv_queue);
			kcp->nrcv_que++; //更新接收队列计数
			kcp->rcv_nxt++; //更新"段编号"
		}	else {
			break;
		}
	}

	/*! 重置接收状态 */
	// fast recover
	if (kcp->nrcv_que < kcp->rcv_wnd && recover) {
		// ready to send back IKCP_CMD_WINS in ikcp_flush
		// tell remote my window size
		/*! 通知远端窗口大小 */
		kcp->probe |= IKCP_ASK_TELL;
	}

	return len;
}


/*! 该函数主要探测接收队列中，数据的总长度 */
//---------------------------------------------------------------------
// peek data size
//---------------------------------------------------------------------
int ikcp_peeksize(const ikcpcb *kcp)
{
	struct IQUEUEHEAD *p;
	IKCPSEG *seg;
	int length = 0;

	assert(kcp);

	if (iqueue_is_empty(&kcp->rcv_queue)) return -1;

	/*! 取出分片 */
	seg = iqueue_entry(kcp->rcv_queue.next, IKCPSEG, node);
	/*! 判断分片是否为最后一个 */
	if (seg->frg == 0) return seg->len;

	/*! 判断接收队列的数量是否小于分片编号 */
	if (kcp->nrcv_que < seg->frg + 1) return -1;

	/*! 探查当前接收队列中数据的长度 */
	for (p = kcp->rcv_queue.next; p != &kcp->rcv_queue; p = p->next) {
		seg = iqueue_entry(p, IKCPSEG, node);
		length += seg->len;
		if (seg->frg == 0) break;
	}

	return length;
}


/*! 将buffer中的数据转换为分片并添加到"发送队列"中 */
//---------------------------------------------------------------------
// user/upper level send, returns below zero for error
//---------------------------------------------------------------------
int ikcp_send(ikcpcb *kcp, const char *buffer, int len)
{
	IKCPSEG *seg;
	int count, i;

	/*! 判断分片大小 */
	assert(kcp->mss > 0);
	/*! 判断外部发送buffer长度 */
	if (len < 0) return -1;

	// 当为流模式时将会从还未发送的数据分片中取出最后一个填充
	// append to previous segment in streaming mode (if possible)
	if (kcp->stream != 0) {
		if (!iqueue_is_empty(&kcp->snd_queue)) {
			/*! 以下工作主要是判断前一个分片的数据长度是否小于分片限制"mss" */
			/*! 将前一个分片与当前数据合并为一个分片 */
			IKCPSEG *old = iqueue_entry(kcp->snd_queue.prev, IKCPSEG, node);
			if (old->len < kcp->mss) {
				int capacity = kcp->mss - old->len;
				int extend = (len < capacity)? len : capacity;
				//分配新分片=>其大小为old数据大小+当前buffer（len）大小
				seg = ikcp_segment_new(kcp, old->len + extend);
				assert(seg);
				if (seg == NULL) {
					return -2;
				}
				/*! 尾部追加当前新的合并分片到发送队列中 */
				iqueue_add_tail(&seg->node, &kcp->snd_queue);
				memcpy(seg->data, old->data, old->len);
				if (buffer) {
					memcpy(seg->data + old->len, buffer, extend);
					buffer += extend;
				}
				seg->len = old->len + extend;
				seg->frg = 0;
				len -= extend;
				iqueue_del_init(&old->node);
				ikcp_segment_delete(kcp, old);
			}
		}
		if (len <= 0) {
			return 0;
		}
	}

	/*! 计算应该分片的数量 */
	if (len <= (int)kcp->mss) count = 1;
	else count = (len + kcp->mss - 1) / kcp->mss;

	/*! 判断接收窗口 */
	if (count >= IKCP_WND_RCV) return -2;

	if (count == 0) count = 1;

	// fragment
	for (i = 0; i < count; i++) {
		/*! 进行分片 */
		int size = len > (int)kcp->mss ? (int)kcp->mss : len;
		seg = ikcp_segment_new(kcp, size);
		assert(seg);
		if (seg == NULL) {
			return -2;
		}
		/*! copy数据到segment */
		if (buffer && len > 0) {
			memcpy(seg->data, buffer, size);
		}
		seg->len = size;
		/*! 如果是流模式则其分片编号为0，否则为相应数量编号 */
		seg->frg = (kcp->stream == 0)? (count - i - 1) : 0;
		iqueue_init(&seg->node);
		/*! 尾部追加数量到发送队列 */
		iqueue_add_tail(&seg->node, &kcp->snd_queue);
		kcp->nsnd_que++; //更新计数

		/*! 偏移数据指向 */
		if (buffer) {
			buffer += size;
		}
		len -= size;
	}

	return 0;
}

/**
 * rx_srtt: smoothed round trip time，平滑后的RTT
 * rx_rttval：RTT的变化量，代表连接的抖动情况
 * interval：内部flush刷新间隔，对系统循环效率有非常重要影响
 * 
 * 该函数主要意图在于更新与ack有关的RTO时间
 * 	RTO相关：通过请求应答时间（RTT）计算出超时重传时间（RTO）
 */
//---------------------------------------------------------------------
// parse ack
//---------------------------------------------------------------------
static void ikcp_update_ack(ikcpcb *kcp, IINT32 rtt)
{
	IINT32 rto = 0;
	if (kcp->rx_srtt == 0) {
		kcp->rx_srtt = rtt;
		kcp->rx_rttval = rtt / 2;
	}	else {
		//平滑抖动算法
		long delta = rtt - kcp->rx_srtt;
		/*! 取正=> abs */
		if (delta < 0) delta = -delta;
		kcp->rx_rttval = (3 * kcp->rx_rttval + delta) / 4;
		kcp->rx_srtt = (7 * kcp->rx_srtt + rtt) / 8;
		if (kcp->rx_srtt < 1) kcp->rx_srtt = 1;
	}
	/*! 通过抖动情况与内部调度间隔计算出RTO时间 */
	rto = kcp->rx_srtt + _imax_(kcp->interval, 4 * kcp->rx_rttval);
	/*! 使得最后结果在minrto <= x <=  IKCP_RTO_MAX 之间 */
	kcp->rx_rto = _ibound_(kcp->rx_minrto, rto, IKCP_RTO_MAX);
}

/**
 * 收缩数据buff
 * una: 此编号前所有包已收到
 */
static void ikcp_shrink_buf(ikcpcb *kcp)
{
	struct IQUEUEHEAD *p = kcp->snd_buf.next;
	/*! 判断发送队列不为空，与iqueue_is_empty一个意思 */
	if (p != &kcp->snd_buf) {
		IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
		/*! 取发送队列中最新分片的编号 */
		kcp->snd_una = seg->sn;
	}	else {
		kcp->snd_una = kcp->snd_nxt;
	}
}

/**
 * 解析ack
 * 该函数主要工作从发送buf中删除相应编号的分片
 */
static void ikcp_parse_ack(ikcpcb *kcp, IUINT32 sn)
{
	struct IQUEUEHEAD *p, *next;

	/*! 当前确认数据包ack的编号小于已经接收到的编号(una)或数据包的ack编号大于待分配的编号则不合法 */
	if (_itimediff(sn, kcp->snd_una) < 0 || _itimediff(sn, kcp->snd_nxt) >= 0)
		return;

	/**
	 * 遍历发送队列释放该编号分片
	 */
	for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = next) {
		IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
		next = p->next;
		if (sn == seg->sn) {
			iqueue_del(p);
			ikcp_segment_delete(kcp, seg);
			kcp->nsnd_buf--;
			break;
		}
		/*! 如果该编号小于则表明不需要继续遍历下去 */
		if (_itimediff(sn, seg->sn) < 0) {
			break;
		}
	}
}

/**
 * 解析una
 * 该函数主要从发送buf中删除已经被确认的数据分片
 */
static void ikcp_parse_una(ikcpcb *kcp, IUINT32 una)
{
	struct IQUEUEHEAD *p, *next;
	/*! 遍历发送buf，并释放已经被确认的分片 */
	for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = next) {
		IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
		next = p->next;
		/*! 发送队列满足sn由大到小的顺序规则 */
		if (_itimediff(una, seg->sn) > 0) {
			iqueue_del(p);
			ikcp_segment_delete(kcp, seg);
			kcp->nsnd_buf--;
		}	else {
			break;
		}
	}
}

/**
 * 解析fastack
 */
static void ikcp_parse_fastack(ikcpcb *kcp, IUINT32 sn)
{
	struct IQUEUEHEAD *p, *next;

	/*! 当前确认数据包ack的编号小于已经接收到的编号(una)或数据包的ack编号大于待分配的编号则不合法 */
	if (_itimediff(sn, kcp->snd_una) < 0 || _itimediff(sn, kcp->snd_nxt) >= 0)
		return;

	/*! 遍历发送buf，进行快速确认(ack) */
	for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = next) {
		IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
		next = p->next;
		if (_itimediff(sn, seg->sn) < 0) {
			break;
		}
		else if (sn != seg->sn) {
			seg->fastack++;
		}
	}
}


/**
 * ts: message发送时刻的时间戳
 * sn: 分片编号
 * ackcount: acklist中ack的数量，每个ack在acklist中存储ts，sn两个量
 * ackblock: 2的倍数，标识acklist最大可容纳的ack数量
 * 该函数主要用与添加ack确认数据包信息
 */
//---------------------------------------------------------------------
// ack append
//---------------------------------------------------------------------
static void ikcp_ack_push(ikcpcb *kcp, IUINT32 sn, IUINT32 ts)
{
	size_t newsize = kcp->ackcount + 1;
	IUINT32 *ptr;

	//判断是否超出acklist可容纳的数量
	if (newsize > kcp->ackblock) {
		IUINT32 *acklist;
		size_t newblock;

		//进行扩容，以8的N次方扩充
		for (newblock = 8; newblock < newsize; newblock <<= 1);
		//分配数组
		acklist = (IUINT32*)ikcp_malloc(newblock * sizeof(IUINT32) * 2);

		//分配失败了....现代操作系统基本不会分配失败，但是存在此问题
		if (acklist == NULL) {
			assert(acklist != NULL);
			abort();
		}

		/*! 不为空则需要copy */
		if (kcp->acklist != NULL) {
			size_t x;
			for (x = 0; x < kcp->ackcount; x++) {
				//由于是交错着的数据类型1,2,1,2,1,2
				acklist[x * 2 + 0] = kcp->acklist[x * 2 + 0]; //sn
				acklist[x * 2 + 1] = kcp->acklist[x * 2 + 1]; //ts
			}
			//释放旧数据
			ikcp_free(kcp->acklist);
		}

		//数组赋值
		kcp->acklist = acklist;
		//容量赋值
		kcp->ackblock = newblock;
	}

	//进行数组下标偏移
	ptr = &kcp->acklist[kcp->ackcount * 2];
	ptr[0] = sn; 
	ptr[1] = ts;
	kcp->ackcount++; //增加数量
}

/*! 通过下标偏移获取相应位置p上的ack确认包数据信息 */
static void ikcp_ack_get(const ikcpcb *kcp, int p, IUINT32 *sn, IUINT32 *ts)
{
	if (sn) sn[0] = kcp->acklist[p * 2 + 0];
	if (ts) ts[0] = kcp->acklist[p * 2 + 1];
}

/**
 * 解析数据
 * 该函数主要将数据分片追加到buf中，并将buf中数据有序的移置接收队列中
 */
//---------------------------------------------------------------------
// parse data
//---------------------------------------------------------------------
void ikcp_parse_data(ikcpcb *kcp, IKCPSEG *newseg)
{
	struct IQUEUEHEAD *p, *prev;
	IUINT32 sn = newseg->sn;
	int repeat = 0;
	
	/**
	 * 判断该数据分片的编号是否超出接收窗口可接收的范围，或者
	 * 该编号小于需要的则直接丢弃
	 */
	if (_itimediff(sn, kcp->rcv_nxt + kcp->rcv_wnd) >= 0 ||
		_itimediff(sn, kcp->rcv_nxt) < 0) {
		ikcp_segment_delete(kcp, newseg);
		return;
	}

	/*! 在接收buf中寻找编号为sn的分片用来判断是否重复 */
	for (p = kcp->rcv_buf.prev; p != &kcp->rcv_buf; p = prev) {
		IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
		prev = p->prev;
	
		if (seg->sn == sn) {
			//重复了
			repeat = 1;
			break;
		}
		//由于分片编号是递增的
		if (_itimediff(sn, seg->sn) > 0) {
			break;
		}
	}

	if (repeat == 0) {
		//将该分片添加到发送buf中
		iqueue_init(&newseg->node);
		//头部追加
		iqueue_add(&newseg->node, p);
		kcp->nrcv_buf++;
	}	else {
		//重复则删除该分片
		ikcp_segment_delete(kcp, newseg);
	}

#if 0
	ikcp_qprint("rcvbuf", &kcp->rcv_buf);
	printf("rcv_nxt=%lu\n", kcp->rcv_nxt);
#endif

	/*! 将数据从接收buf中移置接收队列中 */
	// move available data from rcv_buf -> rcv_queue
	while (! iqueue_is_empty(&kcp->rcv_buf)) {
		IKCPSEG *seg = iqueue_entry(kcp->rcv_buf.next, IKCPSEG, node);
		/*! 该判断使得在rcv_queue中的数据分片是有序的 */
		/*! 接收队列的数量应该小于接收窗口的限制 */
		if (seg->sn == kcp->rcv_nxt && kcp->nrcv_que < kcp->rcv_wnd) {
			iqueue_del(&seg->node);
			kcp->nrcv_buf--;
			iqueue_add_tail(&seg->node, &kcp->rcv_queue);
			kcp->nrcv_que++;
			kcp->rcv_nxt++;
		}	else {
			break;
		}
	}

#if 0
	ikcp_qprint("queue", &kcp->rcv_queue);
	printf("rcv_nxt=%lu\n", kcp->rcv_nxt);
#endif

#if 1
//	printf("snd(buf=%d, queue=%d)\n", kcp->nsnd_buf, kcp->nsnd_que);
//	printf("rcv(buf=%d, queue=%d)\n", kcp->nrcv_buf, kcp->nrcv_que);
#endif
}



/**
 * 数据输入
 * 该函数主要是处理接收到的数据
 * 校验数据=》解析数据=》处理数据（将合法的数据分片添加到接收buf中）=》拥塞窗口处理
 * 
 */
//---------------------------------------------------------------------
// input data
//---------------------------------------------------------------------
int ikcp_input(ikcpcb *kcp, const char *data, long size)
{
	IUINT32 una = kcp->snd_una;
	IUINT32 maxack = 0;
	int flag = 0;

	if (ikcp_canlog(kcp, IKCP_LOG_INPUT)) {
		ikcp_log(kcp, IKCP_LOG_INPUT, "[RI] %d bytes", size);
	}

	/*! 数据和长度的初步校验 */
	if (data == NULL || (int)size < (int)IKCP_OVERHEAD) return -1;

	while (1) {
		IUINT32 ts, sn, len, una, conv;
		IUINT16 wnd;
		IUINT8 cmd, frg;
		IKCPSEG *seg;

		if (size < (int)IKCP_OVERHEAD) break;

		/*! 校验数据分片 */
		//取4字节数据
		data = ikcp_decode32u(data, &conv);
		// !!! modify !!!
		// if (conv != kcp->conv) return -1;
		conv = kcp->conv;

		data = ikcp_decode8u(data, &cmd);
		data = ikcp_decode8u(data, &frg);
		data = ikcp_decode16u(data, &wnd);
		data = ikcp_decode32u(data, &ts);
		data = ikcp_decode32u(data, &sn);
		data = ikcp_decode32u(data, &una);
		data = ikcp_decode32u(data, &len);

		//剔除固定的包头信息长度
		size -= IKCP_OVERHEAD;

		if ((long)size < (long)len || (int)len < 0) return -2;

		//只支持{IKCP_CMD_PUSH, IKCP_CMD_ACK, IKCP_CMD_WASK, IKCP_CMD_WINS}指令
		//其他不合法
		if (cmd != IKCP_CMD_PUSH && cmd != IKCP_CMD_ACK &&
			cmd != IKCP_CMD_WASK && cmd != IKCP_CMD_WINS) 
			return -3;

		/*! 每个数据分片都会具有远端接收窗口大小 */
		kcp->rmt_wnd = wnd;
		//遍历当前发送buf是否具有该una编号
		//若具有则从发送buf中移除, 表明该una已经被确认了
		ikcp_parse_una(kcp, una);
		//收缩发送buf
		ikcp_shrink_buf(kcp);

		/*! ACK指令 */
		if (cmd == IKCP_CMD_ACK) {
			//接收确认ack将相应sn的数据分片从发送buf中移除
			//判断当前时间与ts(发送时间戳)
			if (_itimediff(kcp->current, ts) >= 0) {
				ikcp_update_ack(kcp, _itimediff(kcp->current, ts));
			}
			//遍历当前发送buf是否具有该sn编号
			ikcp_parse_ack(kcp, sn);
			//收缩发送buf
			ikcp_shrink_buf(kcp);
			if (flag == 0) {
				flag = 1;
				maxack = sn;
			}	else {
				if (_itimediff(sn, maxack) > 0) {
					maxack = sn;
				}
			}
			if (ikcp_canlog(kcp, IKCP_LOG_IN_ACK)) {
				ikcp_log(kcp, IKCP_LOG_IN_DATA, 
					"input ack: sn=%lu rtt=%ld rto=%ld", sn, 
					(long)_itimediff(kcp->current, ts),
					(long)kcp->rx_rto);
			}
		}
		//接收到具体的数据包
		else if (cmd == IKCP_CMD_PUSH) {
			if (ikcp_canlog(kcp, IKCP_LOG_IN_DATA)) {
				ikcp_log(kcp, IKCP_LOG_IN_DATA, 
					"input psh: sn=%lu ts=%lu", sn, ts);
			}
			/*! 判断接收的数据分片编号是否符合要求，即：在接收窗口（滑动窗口）范围之内 */
			if (_itimediff(sn, kcp->rcv_nxt + kcp->rcv_wnd) < 0) {
				//添加该数据分片编号的ack确认包进acklist中
				ikcp_ack_push(kcp, sn, ts);

				/*! 如果该分片编号在等待接收的范围内 */
				if (_itimediff(sn, kcp->rcv_nxt) >= 0) {
					//分配新的数据分片并拷贝
					seg = ikcp_segment_new(kcp, len);
					seg->conv = conv;
					seg->cmd = cmd;
					seg->frg = frg;
					seg->wnd = wnd;
					seg->ts = ts;
					seg->sn = sn;
					seg->una = una;
					seg->len = len;

					if (len > 0) {
						memcpy(seg->data, data, len);
					}
					//将该分片添加到接收buf中，可能会添加到接收队列中
					ikcp_parse_data(kcp, seg);
				}
			}
		}
		else if (cmd == IKCP_CMD_WASK) {
			// 如果是探测包
			// ready to send back IKCP_CMD_WINS in ikcp_flush
			// tell remote my window size
			//添加相应的标识位
			kcp->probe |= IKCP_ASK_TELL;
			if (ikcp_canlog(kcp, IKCP_LOG_IN_PROBE)) {
				ikcp_log(kcp, IKCP_LOG_IN_PROBE, "input probe");
			}
		}
		else if (cmd == IKCP_CMD_WINS) {
			// 如果是tell me 远端窗口大小，什么都不做
			// do nothing
			if (ikcp_canlog(kcp, IKCP_LOG_IN_WINS)) {
				ikcp_log(kcp, IKCP_LOG_IN_WINS,
					"input wins: %lu", (IUINT32)(wnd));
			}
		}
		else {
			return -3;
		}
		//进行指针偏移=》此时指向下一个分片
		data += len;
		size -= len; //收缩未校验数据长度
	}
	//表示maxack有效
	if (flag != 0) {
		/*! 更新一下可能丢包的记数 */
		ikcp_parse_fastack(kcp, maxack);
	}

	//如果当前记录的una（该编号之前的数据都已经被远端确认）大于当前数据分片的una
	if (_itimediff(kcp->snd_una, una) > 0) {
		//如何拥塞窗口小于远端窗口
		if (kcp->cwnd < kcp->rmt_wnd) {
			//最大分片大小
			IUINT32 mss = kcp->mss;
			//拥塞窗口小于阈值
			if (kcp->cwnd < kcp->ssthresh) {
				kcp->cwnd++;
				kcp->incr += mss;
			}	else {
				if (kcp->incr < mss) kcp->incr = mss; //最小边界

				kcp->incr += (mss * mss) / kcp->incr + (mss / 16);
				if ((kcp->cwnd + 1) * mss <= kcp->incr) {
					kcp->cwnd++;
				}
			}
			//如果拥塞窗口大于远端窗口
			if (kcp->cwnd > kcp->rmt_wnd) {
				//则使用远端窗口
				kcp->cwnd = kcp->rmt_wnd;
				//并设置相应数据量，该数据量以字节数
				kcp->incr = kcp->rmt_wnd * mss;
			}
		}
	}

	return 0;
}


/**
 * 打包分片
 */
//---------------------------------------------------------------------
// ikcp_encode_seg
//---------------------------------------------------------------------
static char *ikcp_encode_seg(char *ptr, const IKCPSEG *seg)
{
	ptr = ikcp_encode32u(ptr, seg->conv);
	ptr = ikcp_encode8u(ptr, (IUINT8)seg->cmd);
	ptr = ikcp_encode8u(ptr, (IUINT8)seg->frg);
	ptr = ikcp_encode16u(ptr, (IUINT16)seg->wnd);
	ptr = ikcp_encode32u(ptr, seg->ts);
	ptr = ikcp_encode32u(ptr, seg->sn);
	ptr = ikcp_encode32u(ptr, seg->una);
	ptr = ikcp_encode32u(ptr, seg->len);
	return ptr;
}

/*! 计算可接收长度，以包为单位 */
static int ikcp_wnd_unused(const ikcpcb *kcp)
{
	//接收队列中数据量如果小于接收窗口
	if (kcp->nrcv_que < kcp->rcv_wnd) {
		return kcp->rcv_wnd - kcp->nrcv_que; //则返回可再接收长度
	}
	return 0;
}


/**
 * 该函数主要职责：发送数据、更新状态
 * 1. ack确认包
 * 2. 探测远端窗口
 * 3. 发送buf数据分片
 * 4. 更新拥塞窗口
 */
//---------------------------------------------------------------------
// ikcp_flush
//---------------------------------------------------------------------
void ikcp_flush(ikcpcb *kcp)
{
	IUINT32 current = kcp->current;
	char *buffer = kcp->buffer;
	char *ptr = buffer;
	int count, size, i;
	/*! resent: 快送重传数 */
	IUINT32 resent, cwnd;
	IUINT32 rtomin;
	struct IQUEUEHEAD *p;
	int change = 0;
	int lost = 0;
	IKCPSEG seg;

	// 'ikcp_update' haven't been called. 
	if (kcp->updated == 0) return;

	/*! 发送ack确认包 */
	seg.conv = kcp->conv;
	seg.cmd = IKCP_CMD_ACK;
	seg.frg = 0;
	seg.wnd = ikcp_wnd_unused(kcp); //未使用的窗口长度
	seg.una = kcp->rcv_nxt; //
	seg.len = 0;
	seg.sn = 0;
	seg.ts = 0;

	/*! 将ack分片打包到buf中 */
	// flush acknowledges
	count = kcp->ackcount;
	for (i = 0; i < count; i++) {
		size = (int)(ptr - buffer);
		if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
			ikcp_output(kcp, buffer, size);
			ptr = buffer;
		}
		//获取相应ackcount的编号对应的分片编号和时间戳
		ikcp_ack_get(kcp, i, &seg.sn, &seg.ts);
		//打包分片到buf中
		ptr = ikcp_encode_seg(ptr, &seg);
	}

	//清空ack数量
	kcp->ackcount = 0;

	// 远端窗口为0则需要探测一下
	// probe window size (if remote window size equals zero)
	if (kcp->rmt_wnd == 0) {
		if (kcp->probe_wait == 0) {
			kcp->probe_wait = IKCP_PROBE_INIT; //初始化探测时长
			kcp->ts_probe = kcp->current + kcp->probe_wait; //设置探测时间戳
		}	
		else {
			//当前时间大于与探测时间戳=》探测超时
			if (_itimediff(kcp->current, kcp->ts_probe) >= 0) {
				if (kcp->probe_wait < IKCP_PROBE_INIT)  //探测时长下限
					kcp->probe_wait = IKCP_PROBE_INIT;

				//探测时长增加一半
				kcp->probe_wait += kcp->probe_wait / 2;
				
				//探测时长上限
				if (kcp->probe_wait > IKCP_PROBE_LIMIT)
					kcp->probe_wait = IKCP_PROBE_LIMIT;
				
				//设置探测时间戳
				kcp->ts_probe = kcp->current + kcp->probe_wait;

				//增加ask（窗口探测包）发送标识
				kcp->probe |= IKCP_ASK_SEND;
			}
		}
	}	else {
		//远端窗口正常，则不需要探测
		kcp->ts_probe = 0;
		kcp->probe_wait = 0;
	}

	// 判断探测标识
	// flush window probing commands
	if (kcp->probe & IKCP_ASK_SEND) {
		//设置分片指令标识
		seg.cmd = IKCP_CMD_WASK;
		size = (int)(ptr - buffer);
		//判断使用空间+固定头部信息的长度是否超过最大的传输单元
		if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
			ikcp_output(kcp, buffer, size); //输出buf
			ptr = buffer; //更新指向
		}
		//打包数据
		ptr = ikcp_encode_seg(ptr, &seg);
	}
	// 判断回馈探测窗口标识
	// flush window probing commands
	if (kcp->probe & IKCP_ASK_TELL) {
		//设置分片指令标识
		seg.cmd = IKCP_CMD_WINS;
		//判断使用空间+固定头部信息的长度是否超过最大的传输单元
		size = (int)(ptr - buffer);
		if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
			ikcp_output(kcp, buffer, size);//输出buf
			ptr = buffer; //更新指向
		}
		//打包数据
		ptr = ikcp_encode_seg(ptr, &seg);
	}
	//清空标识
	kcp->probe = 0;

	// 取发送窗口和远端窗口最小值得到拥塞窗口小
	// calculate window size
	cwnd = _imin_(kcp->snd_wnd, kcp->rmt_wnd);

	// 如果没有做流控制则取配置拥塞窗口、发送窗口和远端窗口三者最小值
	if (kcp->nocwnd == 0) cwnd = _imin_(kcp->cwnd, cwnd);
	// 拥塞窗口以包为单位
	
	// 将发送队列中的数据移植发送buf中
	// move data from snd_queue to snd_buf
	// 发送那些符合拥塞范围的数据分片
	while (_itimediff(kcp->snd_nxt, kcp->snd_una + cwnd) < 0) {
		IKCPSEG *newseg;
		if (iqueue_is_empty(&kcp->snd_queue)) break;

		newseg = iqueue_entry(kcp->snd_queue.next, IKCPSEG, node);

		iqueue_del(&newseg->node);
		iqueue_add_tail(&newseg->node, &kcp->snd_buf);
		kcp->nsnd_que--;
		kcp->nsnd_buf++;
		//设置数据分片的属性
		newseg->conv = kcp->conv; // 会话编号
		newseg->cmd = IKCP_CMD_PUSH; // 表示该分片为具体的数据
		newseg->wnd = seg.wnd; // 使用接收窗口大小
		newseg->ts = current; // 发送时间戳
		newseg->sn = kcp->snd_nxt++; // 分片编号
		newseg->una = kcp->rcv_nxt; // 设置una
		newseg->resendts = current; // 重发时间戳
		newseg->rto = kcp->rx_rto; // 超时重传时间
		newseg->fastack = 0; //
		newseg->xmit = 0;
	}

	// calculate resent
	resent = (kcp->fastresend > 0)? (IUINT32)kcp->fastresend : 0xffffffff;
	rtomin = (kcp->nodelay == 0)? (kcp->rx_rto >> 3) : 0;

	// 将分片buf数据发送出去
	// flush data segments
	for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = p->next) {
		IKCPSEG *segment = iqueue_entry(p, IKCPSEG, node);
		int needsend = 0;
		//该分片首次发送
		if (segment->xmit == 0) {
			needsend = 1;
			segment->xmit++;
			segment->rto = kcp->rx_rto;
			segment->resendts = current + segment->rto + rtomin;
		}
		//该分片的超时重传已经到达
		else if (_itimediff(current, segment->resendts) >= 0) {
			needsend = 1;
			segment->xmit++;
			kcp->xmit++;
			if (kcp->nodelay == 0) {
				segment->rto += kcp->rx_rto;
			}	else {
				segment->rto += kcp->rx_rto / 2;
			}
			segment->resendts = current + segment->rto;
			lost = 1; //发现丢失、设置丢失标识
		}
		//判断是否丢包
		else if (segment->fastack >= resent) {
			needsend = 1;
			segment->xmit++;
			segment->fastack = 0; //重置被跳过次数标识
			segment->resendts = current + segment->rto; //重新设置超时时间戳
			change++;
		}

		/*! 需要发送数据包 */
		if (needsend) {
			int size, need;
			segment->ts = current; //发送时的时间戳
			segment->wnd = seg.wnd; //窗口大小
			segment->una = kcp->rcv_nxt; //该编号前的数据包都已经被ack确认
			//判断使用空间+固定头部信息+分片大小的长度是否超过最大的传输单元
			size = (int)(ptr - buffer);
			need = IKCP_OVERHEAD + segment->len;

			if (size + need > (int)kcp->mtu) {
				//超出则输出
				ikcp_output(kcp, buffer, size);
				ptr = buffer;
			}

			//打包分片数据
			ptr = ikcp_encode_seg(ptr, segment);

			//指针偏移
			if (segment->len > 0) {
				memcpy(ptr, segment->data, segment->len);
				ptr += segment->len;
			}

			//判断该分片重传次数是否大于最大重传次数
			if (segment->xmit >= kcp->dead_link) {
				kcp->state = -1; //连接状态 0xFFFFFFFF == -1 表示断开连接
			}
		}
	}

	/*! 发送剩余数据 */
	// flash remain segments
	size = (int)(ptr - buffer);
	if (size > 0) {
		ikcp_output(kcp, buffer, size);
	}

	// 更新阀值
	// update ssthresh
	if (change) {
		//下一个发送分片的编号与下一个未被确认的编号，得出相差几个包
		//未确认包的个数
		IUINT32 inflight = kcp->snd_nxt - kcp->snd_una;
		kcp->ssthresh = inflight / 2; //得出阀值，即未确认包数量的一半
		if (kcp->ssthresh < IKCP_THRESH_MIN) //下限阀值
			kcp->ssthresh = IKCP_THRESH_MIN;
		
		//设置拥塞窗口（以包为单位），即：阀值 + 快速重传数
		kcp->cwnd = kcp->ssthresh + resent;
		//通过拥塞窗口得出发送数据量，以字节为单位
		kcp->incr = kcp->cwnd * kcp->mss;
	}

	//丢失标识
	if (lost) {
		//产生丢包，则通过拥塞窗口确定阀值
		kcp->ssthresh = cwnd / 2;
		if (kcp->ssthresh < IKCP_THRESH_MIN)
			kcp->ssthresh = IKCP_THRESH_MIN;
		kcp->cwnd = 1;
		kcp->incr = kcp->mss;
	}

	//设置拥塞窗口下限
	if (kcp->cwnd < 1) {
		kcp->cwnd = 1;
		kcp->incr = kcp->mss;
	}
}


//---------------------------------------------------------------------
// update state (call it repeatedly, every 10ms-100ms), or you can ask 
// ikcp_check when to call it again (without ikcp_input/_send calling).
// 'current' - current timestamp in millisec. 
//---------------------------------------------------------------------
void ikcp_update(ikcpcb *kcp, IUINT32 current)
{
	IINT32 slap;

	kcp->current = current;

	if (kcp->updated == 0) {
		kcp->updated = 1;
		kcp->ts_flush = kcp->current;
	}

	//判断当前update时间与刷新时间戳
	slap = _itimediff(kcp->current, kcp->ts_flush);

	//如果相差10s
	if (slap >= 10000 || slap < -10000) {
		//直接刷新
		kcp->ts_flush = kcp->current;
		slap = 0;
	}

	//已经到了刷新时间
	if (slap >= 0) {
		//设置下次刷新时间戳
		kcp->ts_flush += kcp->interval;
		//保证没错
		if (_itimediff(kcp->current, kcp->ts_flush) >= 0) {
			kcp->ts_flush = kcp->current + kcp->interval;
		}
		//调用刷新，将数据发送出去
		ikcp_flush(kcp);
	}
}


/**
 * 得到下次合适调用ikcp_update是时间
 */
//---------------------------------------------------------------------
// Determine when should you invoke ikcp_update:
// returns when you should invoke ikcp_update in millisec, if there 
// is no ikcp_input/_send calling. you can call ikcp_update in that
// time, instead of call update repeatly.
// Important to reduce unnacessary ikcp_update invoking. use it to 
// schedule ikcp_update (eg. implementing an epoll-like mechanism, 
// or optimize ikcp_update when handling massive kcp connections)
//---------------------------------------------------------------------
IUINT32 ikcp_check(const ikcpcb *kcp, IUINT32 current)
{
	IUINT32 ts_flush = kcp->ts_flush;
	IINT32 tm_flush = 0x7fffffff;
	IINT32 tm_packet = 0x7fffffff;
	IUINT32 minimal = 0;
	struct IQUEUEHEAD *p;

	/*! 如果更新标识未被调用则直接返回 */
	if (kcp->updated == 0) {
		return current;
	}

	/*! 如果超时10s，则直接调度 */
	if (_itimediff(current, ts_flush) >= 10000 ||
		_itimediff(current, ts_flush) < -10000) {
		ts_flush = current;
	}

	//直接返回
	if (_itimediff(current, ts_flush) >= 0) {
		return current;
	}


	//得出差值
	tm_flush = _itimediff(ts_flush, current);

	//遍历发送buf
	for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = p->next) {
		const IKCPSEG *seg = iqueue_entry(p, const IKCPSEG, node);
		//寻找重新时间戳到期的数据
		IINT32 diff = _itimediff(seg->resendts, current);
		if (diff <= 0) {
			return current;
		}
		//设置数据分片距离当前时间最近的差值
		if (diff < tm_packet) tm_packet = diff;
	}

	//比较调度时间差与最近数据分片差值
	minimal = (IUINT32)(tm_packet < tm_flush ? tm_packet : tm_flush);
	if (minimal >= kcp->interval) minimal = kcp->interval;

	//得出最近调度时间戳
	return current + minimal;
}


/*! 设置最大传输单元 */
int ikcp_setmtu(ikcpcb *kcp, int mtu)
{
	char *buffer;
	//设置数值不合法
	if (mtu < 50 || mtu < (int)IKCP_OVERHEAD) 
		return -1;
	//分配3倍的传输buf
	buffer = (char*)ikcp_malloc((mtu + IKCP_OVERHEAD) * 3);
	if (buffer == NULL) 
		return -2;
	
	//设置相应属性
	kcp->mtu = mtu;
	//设置最大分片大小
	kcp->mss = kcp->mtu - IKCP_OVERHEAD;

	ikcp_free(kcp->buffer);
	kcp->buffer = buffer;
	return 0;
}

/*! 设置调度间隔 */
int ikcp_interval(ikcpcb *kcp, int interval)
{
	if (interval > 5000) interval = 5000;
	else if (interval < 10) interval = 10;
	kcp->interval = interval;
	return 0;
}

/*! 设置无延迟机制 */
int ikcp_nodelay(ikcpcb *kcp, int nodelay, int interval, int resend, int nc)
{
	if (nodelay >= 0) {
		kcp->nodelay = nodelay;
		if (nodelay) {
			//设置无延迟，设置相应的最小重传时间
			kcp->rx_minrto = IKCP_RTO_NDL;
		}	
		else {
			//设置正常的最小重传时间
			kcp->rx_minrto = IKCP_RTO_MIN;
		}
	}
	//设置调度间隔
	if (interval >= 0) {
		if (interval > 5000) interval = 5000;
		else if (interval < 10) interval = 10;
		kcp->interval = interval;
	}
	//设置快速重传数
	if (resend >= 0) {
		kcp->fastresend = resend;
	}
	//开启无拥塞模式
	if (nc >= 0) {
		kcp->nocwnd = nc;
	}
	return 0;
}

/*! 设置发送和接收窗口 */
int ikcp_wndsize(ikcpcb *kcp, int sndwnd, int rcvwnd)
{
	if (kcp) {
		if (sndwnd > 0) {
			kcp->snd_wnd = sndwnd;
		}
		if (rcvwnd > 0) {   // must >= max fragment size
			kcp->rcv_wnd = _imax_(rcvwnd, IKCP_WND_RCV);
		}
	}
	return 0;
}

/*! 得出待发送的数量: buf中的分片与发送队列总和 */
int ikcp_waitsnd(const ikcpcb *kcp)
{
	return kcp->nsnd_buf + kcp->nsnd_que;
}

/*! 得出相应的会话编号 */
// read conv
IUINT32 ikcp_getconv(const void *ptr)
{
	IUINT32 conv;
	ikcp_decode32u((const char*)ptr, &conv);
	return conv;
}

#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif
