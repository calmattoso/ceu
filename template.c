#include <string.h>
#include <limits.h>
#include <assert.h>

=== DEFS ===

#define PR_MAX 0x7F
#define PR_MIN (-0x7F)

#ifdef __cplusplus
#define CEU_WCLOCK_NONE 0x7fffffffL     // TODO
#else
#define CEU_WCLOCK_NONE INT32_MAX
#endif

#define PTR(tp,off)     ((tp)(off))
#define PTR_org(tp,off) ((tp)(_trk_.org + off))

#define CEU_NMEM       (=== CEU_NMEM ===)
#define CEU_NTRACKS    (=== CEU_NTRACKS ===)
#define CEU_NLSTS      (=== CEU_NLSTS ===)

// Macros that can be defined:
// ceu_out_pending() (1)
// ceu_out_wclock(us)
// ceu_out_event(id, len, data)

typedef === TCEU_NTRK === tceu_ntrk;    // max number of tracks
typedef === TCEU_NLST === tceu_nlst;    // max number of event listeners
typedef === TCEU_NEVT === tceu_nevt;    // max number of event ids
typedef === TCEU_NLBL === tceu_nlbl;    // max number of label ids

typedef struct {
    void*     org;
    tceu_nlbl lbl;
#ifdef CEU_TRK_PRIO
    s8 prio;
#endif
} tceu_trk;

// TODO: union
typedef struct {
    tceu_nevt evt;
    void*     src;
    void*     org;
    tceu_nlbl lbl;
    s32 togo;
} tceu_lst;

enum {
=== LABELS ===
};

int ceu_go (int* ret);

=== HOST ===

typedef struct {
    tceu_ntrk   tracks_n;
#ifdef CEU_TRK_PRIO
    tceu_trk    tracks[CEU_NTRACKS+1];  // 0 is reserved
#else
    tceu_trk    tracks[CEU_NTRACKS];
#endif

#ifdef CEU_EXTS
    void*       ext_data;
    int         ext_int;
#endif

#ifdef CEU_WCLOCKS
    int         wclk_late;
    s32         wclk_min;
#endif

    tceu_nlst   lsts_n;
    tceu_lst    lsts[CEU_NLSTS];

    char        mem[CEU_NMEM];
} tceu;

tceu CEU = {
    0, {},
#ifdef CEU_EXTS
    0, 0,
#endif
#ifdef CEU_WCLOCKS
    0, CEU_WCLOCK_NONE,
#endif
    0, {},
    {}
};

/**********************************************************************/

#ifdef CEU_TRK_PRIO
    #ifdef CEU_TRK_CHK
        #define ceu_track_ins(chk,prio,org,lbl) ceu_track_ins_YY(chk,prio,org,lbl)
        void ceu_track_ins_YY (int chk, s8 prio, void* org, tceu_nlbl lbl)
    #else
        #define ceu_track_ins(chk,prio,org,lbl) ceu_track_ins_NY(prio,org,lbl)
        void ceu_track_ins_NY (s8 prio, void* org, tceu_nlbl lbl)
    #endif
#else
    #ifdef CEU_TRK_CHK
        #define ceu_track_ins(chk,prio,org,lbl) ceu_track_ins_YN(chk,org,lbl)
        void ceu_track_ins_YN (int chk, void* org, tceu_nlbl lbl)
    #else
        #define ceu_track_ins(chk,prio,org,lbl) ceu_track_ins_NN(org,lbl)
        void ceu_track_ins_NN (void* org, tceu_nlbl lbl)
    #endif
#endif
{
#ifdef CEU_TRK_CHK
    {tceu_ntrk i;
    if (chk) {
        for (i=1; i<=CEU.tracks_n; i++) {
            if (org==CEU.tracks[i].org && lbl==CEU.tracks[i].lbl) {
                return;
            }
        }
    }}
#endif

#ifdef CEU_TRK_PRIO
    {tceu_ntrk i;
    for (i=++CEU.tracks_n; (i>1) && (prio>CEU.tracks[i/2].prio); i/=2)
        CEU.tracks[i] = CEU.tracks[i/2];
    CEU.tracks[i].prio = prio;
    CEU.tracks[i].org  = org;
    CEU.tracks[i].lbl  = lbl;}
#else
    CEU.tracks[CEU.tracks_n++].org = org;
    CEU.tracks[CEU.tracks_n++].lbl = lbl;
#endif
    assert(CEU.tracks_n <= CEU_NTRACKS);        // TODO: remove
}

int ceu_track_rem (tceu_trk* trk, tceu_ntrk N)
{
    if (CEU.tracks_n == 0)
        return 0;

#ifdef CEU_TRK_PRIO
    {tceu_ntrk i,cur;
    tceu_trk* last;

    if (trk)
        *trk = CEU.tracks[1];

    last = &CEU.tracks[CEU.tracks_n--];

    for (i=N; i*2<=CEU.tracks_n; i=cur)
    {
        cur = i * 2;
        if (cur!=CEU.tracks_n && CEU.tracks[cur+1].prio>CEU.tracks[cur].prio)
            cur++;

        if (CEU.tracks[cur].prio>last->prio)
            CEU.tracks[i] = CEU.tracks[cur];
        else
            break;
    }
    CEU.tracks[i] = *last;
    return 1;}
#else
    *trk = CEU.tracks[--CEU.tracks_n];
    return 1;
#endif
}

// TODO: only if escape contains an execute
int ceu_XXX (void* cur, void* org, tceu_nlbl l1, tceu_nlbl l2) {
    void* par = *PTR(void**,cur);
    if (cur == CEU.mem) {
        return 0;                   // root org, no parent
    } else if (par == org) {
        tceu_nlbl lbl = *PTR(tceu_nlbl*,(cur+sizeof(void*)));
        return lbl>=l1 && lbl<=l2;
    } else {
        return ceu_XXX(par, org, l1, l2);
    }
}

void ceu_track_clr (void* org, tceu_nlbl l1, tceu_nlbl l2) {
    tceu_ntrk i;
    for (i=1; i<=CEU.tracks_n; i++) {
        tceu_trk* trk = &CEU.tracks[i];
        if ( trk->lbl>=l1 && trk->lbl<=l2
        ||   trk->org!=org && ceu_XXX(trk->org,org,l1,l2) ) {
            ceu_track_rem(NULL,i);
            i--;
        }
    }
}

/**********************************************************************/

void ceu_lst_ins (tceu_nevt evt, void* src, void* org, tceu_nlbl lbl, s32 togo) {
    CEU.lsts[CEU.lsts_n].evt = evt;
    CEU.lsts[CEU.lsts_n].src = src;
    CEU.lsts[CEU.lsts_n].org = org;
    CEU.lsts[CEU.lsts_n].lbl = lbl;
    CEU.lsts[CEU.lsts_n].togo = togo;
    CEU.lsts_n++;
    assert(CEU.lsts_n <= CEU_NLSTS);        // TODO: remove
}

void ceu_lst_clr (void* org, tceu_nlbl l1, tceu_nlbl l2) {
    tceu_nlst i;
    for (i=0 ; i<CEU.lsts_n ; i++) {
        tceu_lst* lst = &CEU.lsts[i];
        if ( lst->org==org && lst->lbl>=l1 && lst->lbl<=l2
        ||   ceu_XXX(lst->org,org,l1,l2) ) {
            CEU.lsts_n--;
            if (i < CEU.lsts_n) {
                CEU.lsts[i] = CEU.lsts[CEU.lsts_n];
                i--;
            }
        }
    }
}

void ceu_lst_go (tceu_nevt evt, void* src)
{
    tceu_nlst i;
    for (i=0 ; i<CEU.lsts_n ; i++) {
        tceu_lst* lst = &CEU.lsts[i];
        if (lst->evt==evt && lst->src==src) {
            ceu_track_ins(0, PR_MAX, lst->org, lst->lbl);
            (CEU.lsts_n)--;
            if (i < CEU.lsts_n) {
                *lst = CEU.lsts[CEU.lsts_n];
                i--;
            }
        }
    }
}

/**********************************************************************/

#ifdef CEU_EXTS
// returns a pointer to the received value
int* ceu_ext_f (int v) {
    CEU.ext_int = v;
    return &CEU.ext_int;
}
#endif

#ifdef CEU_EMITS
int ceu_track_peek (tceu_trk* trk)
{
    *trk = CEU.tracks[1];
    return CEU.tracks_n > 0;
}
#endif

#ifdef CEU_WCLOCKS

void ceu_wclock_enable (s32 us, void* org, tceu_nlbl lbl) {
    s32 dt = us - CEU.wclk_late;
    ceu_lst_ins(IN__WCLOCK, NULL, org, lbl, dt);
    if (CEU.wclk_min==CEU_WCLOCK_NONE || CEU.wclk_min>dt) {
        CEU.wclk_min = dt;
#ifdef ceu_out_wclock
        ceu_out_wclock(dt);
#endif
    }
}

s32 ceu_wclock_find (void* org, tceu_nlbl lbl) {
    tceu_nlst i;
    for (i=0 ; i<CEU.lsts_n ; i++) {
        if (CEU.lsts[i].org==org && CEU.lsts[i].lbl==lbl) {
            return CEU.lsts[i].togo;
        }
    }
    return CEU_WCLOCK_NONE;
}

#endif

/**********************************************************************/

int ceu_go_init (int* ret)
{
    ceu_track_ins(0, PR_MAX, CEU.mem, Class__Root);
    return ceu_go(ret);
}

#ifdef CEU_EXTS
int ceu_go_event (int* ret, tceu_nevt id, void* data)
{
    CEU.ext_data = data;
    ceu_lst_go(id, 0);

#ifdef CEU_WCLOCKS
    //CEU.wclk_late--;
#endif
    return ceu_go(ret);
}
#endif

#ifdef CEU_ASYNCS
int ceu_go_async (int* ret, int* pending)
{
    int s;

    ceu_lst_go(IN__ASYNC, 0);
#ifdef CEU_WCLOCKS
    //CEU.wclk_late--;
#endif
    s = ceu_go(ret);

    if (pending != NULL)
    {
        tceu_nlst i;
        *pending = 0;
        for (i=0 ; i<CEU.lsts_n ; i++) {
            if (CEU.lsts[i].evt == IN__ASYNC) {
                *pending = 1;
                break;
            }
        }
    }

    return s;
}
#endif

int ceu_go_wclock (int* ret, s32 dt, s32* nxt)
{
#ifdef CEU_WCLOCKS
    tceu_nlst i;
    s32 min_togo = CEU_WCLOCK_NONE;

    if (CEU.wclk_min == CEU_WCLOCK_NONE) {
        if (nxt)
            *nxt = CEU_WCLOCK_NONE;
        return 0;
    }

    if (CEU.wclk_min <= dt) {
        min_togo = CEU.wclk_min;
        CEU.wclk_late = dt - CEU.wclk_min;   // how much late the wclock is
    }

    // spawns all togo/ext
    // finds the next CEU.wclk_min
    // decrements all togo
    CEU.wclk_min = CEU_WCLOCK_NONE;

    for (i=0; i<CEU.lsts_n; i++)
    {
        if (CEU.lsts[i].evt != IN__WCLOCK)
            continue;

        if (CEU.lsts[i].togo == min_togo) {
            ceu_track_ins(0, PR_MAX, CEU.lsts[i].org, CEU.lsts[i].lbl);
            CEU.lsts_n--;
            if (i < CEU.lsts_n) {
                CEU.lsts[i] = CEU.lsts[CEU.lsts_n];
                i--;
            }
        } else {
            CEU.lsts[i].togo -= dt;
            if ( CEU.wclk_min==CEU_WCLOCK_NONE
              || CEU.wclk_min>CEU.lsts[i].togo )
                CEU.wclk_min = CEU.lsts[i].togo;
        }
    }

    {int s = ceu_go(ret);
    if (nxt)
        *nxt = CEU.wclk_min;
    CEU.wclk_late = 0;
    return s;}

#else
    *nxt = CEU_WCLOCK_NONE;
    return 0;
#endif
}

int ceu_go (int* ret)
{
    tceu_trk _trk_;

#ifdef CEU_EMITS
    int _step_ = PR_MIN;
#endif

    while (ceu_track_rem(&_trk_,1))
    {
#ifdef CEU_EMITS
        if (_trk_.prio < 0) {
            tceu_trk T[CEU_NTRACKS+1];
            tceu_ntrk n = 0;
            _step_ = _trk_.prio;
            do {
                T[n++] = _trk_;
            } while ( ceu_track_peek(&_trk_) &&
                      (_trk_.prio>=_step_)   &&
                      ceu_track_rem(NULL,1) );
            for (;n>0;) {
                n--;
                ceu_track_ins(1, PR_MAX, T[n].org, T[n].lbl);
            }
            continue;
        }
#endif

_SWITCH_:
//fprintf(stderr,"TRK: o.%p l.%d\n", _trk_.org, _trk_.lbl);

        switch (_trk_.lbl)
        {
    === CODE ===
        }
    }

    return 0;
}

int ceu_go_all ()
{
    int ret = 0;
#ifdef CEU_ASYNCS
    int async_cnt;
#endif

    if (ceu_go_init(&ret))
        return ret;

#ifdef CEU_ASYNCS
    for (;;) {
        if (ceu_go_async(&ret,&async_cnt))
            return ret;
        if (async_cnt == 0)
            break;              // returns nothing!
    }
#endif

    return ret;
}
