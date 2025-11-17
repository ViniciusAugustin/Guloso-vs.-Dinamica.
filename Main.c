#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

typedef struct {
    int *d;
    size_t n, cap;
} Vec;

static void v_init(Vec *v){ v->d=NULL; v->n=v->cap=0; }
static void v_free(Vec *v){ free(v->d); v->d=NULL; v->n=v->cap=0; }

static int v_push(Vec *v,int x){
    if(v->n==v->cap){
        size_t nc=v->cap? v->cap*2:8;
        int *nd=realloc(v->d,nc*sizeof(int));
        if(!nd) return 0;
        v->d=nd; v->cap=nc;
    }
    v->d[v->n++]=x;
    return 1;
}

static int cmp_desc(const void *a,const void *b){
    int x=*(const int*)a,y=*(const int*)b;
    return y-x;
}

static void dedupe_sort_pos(Vec *v){
    if(!v->n) return;
    qsort(v->d,v->n,sizeof(int),cmp_desc);
    size_t w=0;
    for(size_t i=0;i<v->n;i++){
        int x=v->d[i];
        if(x<=0) continue;
        if(!w || v->d[w-1]!=x) v->d[w++]=x;
    }
    v->n=w;
}

static int parse_coins(const char *s,Vec *v){
    const char *p=s;
    while(*p){
        while(*p && (isspace((unsigned char)*p)||*p==','||*p==';')) p++;
        if(!*p) break;
        char *e=NULL; errno=0;
        long val=strtol(p,&e,10);
        if(p==e){ p++; continue; }
        if(!errno && !v_push(v,(int)val)) return 0;
        p=e;
    }
    dedupe_sort_pos(v);
    return 1;
}

static int read_line(const char *msg,char *buf,size_t sz){
    if(msg){ fputs(msg,stdout); fflush(stdout); }
    if(!fgets(buf,(int)sz,stdin)) return 0;
    size_t n=strlen(buf);
    while(n && (buf[n-1]=='\n'||buf[n-1]=='\r')) buf[--n]='\0';
    return 1;
}

static int reais_to_cents(const char *s,long long *out){
    char tmp[128]; size_t n=strlen(s);
    if(n>=sizeof(tmp)) return 0;
    strcpy(tmp,s);
    for(size_t i=0;tmp[i];i++) if(tmp[i]==',') tmp[i]='.';
    errno=0;
    char *e=NULL; double v=strtod(tmp,&e);
    if(e==tmp || errno || v<0) return 0;
    *out=(long long)llround(v*100.0);
    return 1;
}

static void print_reais(long long c){
    long long a=c>=0?c:-c, r=a/100, ct=a%100;
    if(c<0) printf("-R$ %lld,%02lld",r,ct);
    else    printf("R$ %lld,%02lld",r,ct);
}


/* ---------- Guloso ---------- */
static long long troco_guloso(const Vec *moedas,long long valor,
                              long long *cnt,long long *resto){
    long long r=valor, tot=0;
    for(size_t i=0;i<moedas->n;i++) cnt[i]=0;

    printf("=== Solucao Gulosa ===\n");
    for(size_t i=0;i<moedas->n;i++){
        int m=moedas->d[i];
        if(m<=0) continue;
        long long q=(r>=0)? r/m:0;
        if(q>0){
            printf("%lld moeda(s) de %d centavo(s)\n",q,m);
            cnt[i]=q; tot+=q; r-=q*m;
        }
    }
    printf("\nTotal de moedas usadas (guloso): %lld\n",tot);
    printf("Valor formado pelo guloso: "); print_reais(valor-r); printf("\n");
    if(r){
        printf("Nao foi possivel formar o valor exato com o guloso.\n");
        printf("Valor restante (sobra do guloso): "); print_reais(r); printf("\n\n");
    }else{
        printf("Troco exato obtido com o guloso.\n\n");
    }
    *resto=r;
    return tot;
}


/* ---------- Programacao Dinamica ---------- */
static int troco_pd(const Vec *moedas,long long valor,
                    long long *tot,long long *cnt){
    if(valor<0) return 0;
    if(valor==0){
        for(size_t i=0;i<moedas->n;i++) cnt[i]=0;
        *tot=0;
        printf("=== Solucao com Programacao Dinamica ===\n");
        printf("Valor zero: nenhuma moeda necessaria.\n\n");
        return 1;
    }

    const long long LIM=1000000; /* 10.000,00 */
    if(valor>LIM){
        printf("Valor muito grande para a PD (%lld centavos).\n\n",valor);
        return 0;
    }

    long long V=valor;
    long long *dp=malloc((V+1)*sizeof(long long));
    int *ch=malloc((V+1)*sizeof(int));
    if(!dp||!ch){ free(dp); free(ch); fprintf(stderr,"Falha memoria PD.\n"); return 0; }

    const long long INF=(long long)1e15;
    dp[0]=0; ch[0]=-1;
    for(long long v=1;v<=V;v++){ dp[v]=INF; ch[v]=-1; }

    size_t n=moedas->n;
    for(long long v=1;v<=V;v++)
        for(size_t j=0;j<n;j++){
            int c=moedas->d[j];
            if(c<=0||c>v||dp[v-c]==INF) continue;
            long long cand=dp[v-c]+1;
            if(cand<dp[v]){ dp[v]=cand; ch[v]=(int)j; }
        }

    if(dp[V]==INF){
        printf("=== Solucao com Programacao Dinamica ===\n");
        printf("Nao foi possivel formar o valor exato usando Programacao Dinamica.\n");
        printf("Valor solicitado: "); print_reais(valor);
        printf("\nValor formado (PD): "); print_reais(0);
        printf("\nValor restante (sobra da PD): "); print_reais(valor);
        printf("\n\n");
        free(dp); free(ch); return 0;
    }

    for(size_t i=0;i<n;i++) cnt[i]=0;
    for(long long v=V;v>0;){
        int j=ch[v];
        if(j<0){ free(dp); free(ch); return 0; }
        cnt[j]++; v-=moedas->d[j];
    }

    *tot=dp[V];
    printf("=== Solucao com Programacao Dinamica ===\n");
    for(size_t i=0;i<n;i++)
        if(cnt[i]>0)
            printf("%lld moeda(s) de %d centavo(s)\n",cnt[i],moedas->d[i]);
    printf("\nTotal de moedas usadas (PD): %lld\n",*tot);
    printf("Valor formado (PD): "); print_reais(valor);
    printf("\nTroco exato obtido com PD.\n\n");

    free(dp); free(ch);
    return 1;
}


/* ---------- main ---------- */
int main(void){
    printf("Problema do Troco em Moedas\n");
    printf("Comparacao: Algoritmo Guloso x Programacao Dinamica\n\n");

    char buf[512];
    Vec moedas; v_init(&moedas);

    if(!read_line("Moedas (centavos): ",buf,sizeof(buf)) ||
       !parse_coins(buf,&moedas) || !moedas.n){
        fprintf(stderr,"Erro nas moedas.\n"); v_free(&moedas); return 1;
    }

    if(!read_line("Valor em reais: ",buf,sizeof(buf))){
        fprintf(stderr,"Erro no valor.\n"); v_free(&moedas); return 1;
    }

    long long valor;
    if(!reais_to_cents(buf,&valor)){
        fprintf(stderr,"Valor invalido.\n"); v_free(&moedas); return 1;
    }

    printf("\nMoedas (centavos, decrescente): ");
    for(size_t i=0;i<moedas.n;i++)
        printf("%d%s",moedas.d[i],(i+1==moedas.n)?"":", ");
    printf("\nValor de entrada: "); print_reais(valor); printf("\n\n");

    long long *cg=calloc(moedas.n,sizeof(long long));
    long long *cp=calloc(moedas.n,sizeof(long long));
    if(!cg||!cp){ fprintf(stderr,"Falha memoria contagem.\n");
        free(cg); free(cp); v_free(&moedas); return 1; }

    long long resto_g, tot_g=troco_guloso(&moedas,valor,cg,&resto_g);
    long long tot_p=0;
    int ok_pd=troco_pd(&moedas,valor,&tot_p,cp);
    long long resto_p=ok_pd?0:valor;

    printf("=== Comparacao Guloso x Programacao Dinamica ===\n\n");
    printf("Detalhamento por moeda:\n");
    for(size_t i=0;i<moedas.n;i++){
        printf("%3d centavo(s): Guloso -> %lld, ",
               moedas.d[i],cg[i]);
        printf("PD -> %lld\n", ok_pd?cp[i]:0);
    }
    printf("\nGuloso: %lld moeda(s)",tot_g);
    if(resto_g){
        printf(" e NAO formou o valor exato.\nSobra do guloso: ");
        print_reais(resto_g); printf("\n");
    }else{
        printf(" e formou o valor exato.\n");
    }

    if(ok_pd){
        printf("Programacao Dinamica: %lld moeda(s) e formou o valor exato.\n",tot_p);
    }else{
        printf("Programacao Dinamica NAO formou o valor exato.\nSobra da PD: ");
        print_reais(resto_p); printf("\n");
    }

    if(ok_pd && !resto_g){
        if(tot_p<tot_g)      printf("=> PD usa MENOS moedas que o guloso.\n");
        else if(tot_p==tot_g)printf("=> Ambos usam o MESMO numero de moedas.\n");
        else                 printf("=> Guloso usa menos moedas (caso raro).\n");
    }

    printf("\nFim da execucao.\n");
    free(cg); free(cp); v_free(&moedas);
    return 0;
}
