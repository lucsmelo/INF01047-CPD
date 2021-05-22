#include <iostream>
#include "parser.hpp"
#include <time.h>
#include <string.h>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <locale>
#include <numeric>

 
using namespace aria::csv;
using namespace std;

#define CHAR_TO_INDEX(c) ((int)c - (int)' ')
#define RATING_TAM 151733
#define FILMES_TAM 40277
const int ALPHABET_SIZE = 96;

//com certeza não é a melhor forma de fazer pensando em memória
std::string *names;
vector<string> prefixos;
vector<float> vetorAux;
vector<pair<int,float>> vetorpair;

//Usado para fazer o sort de pares baseado no segundo elemento
template <class T1, class T2, class Pred = std::less<T2> >
struct sort_pair_second
{
    bool operator()(const std::pair<T1,T2>&left, const std::pair<T1,T2>&right)
    {
        Pred p;
        return p(left.second, right.second);
    }
};


//estrutura de dados da para as tags
typedef struct tag
{

    string tag;
    vector<string>movieId;
    vector<string>notas;
    int conf;

} tag;

//Arvore Trie Obrigatoria
struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    string data;
    string movieId;
    string sla;

    // Variável é verdadeira se nodo representa fim da palavra
    bool isEndOfWord;
};

//estrutura de dados da para os filmes
struct filme
{

    string nome;
    string titulo;
    string lista;
    float media;
    int num_ava;
    int num_col;
};
//estrutura de dados da para os as avaliacoes
typedef struct Avaliacoes
{

    string user;
    vector<string>filmes;
    vector<string>notas;
    int media;
    int total_ava;
    int num_col;


    struct Avaliacoes *prox;

} Avaliacoes;

/////////////////////////////////////////////////////////////////////////////

//Tabelas Hash
Avaliacoes* hash_table_rat[RATING_TAM];
tag*hash_table_tag[RATING_TAM];
filme* hash_table[FILMES_TAM];



//////////////////////////////////////////////////////////////////////////////
//																			//
//																			//
//							FUNÇÕES DE HASH									//							
//																			//
//																			//
//																			//
//////////////////////////////////////////////////////////////////////////////

long long polynomialRollingHash(  string const& str)
{
    // P and M
    int p = 31;
    int m = 1e9 + 9;
    long long power_of_p = 1;
    long long hash_val = 0;

    // Loop para calcular o valor da Hash por iteração dos elementos da string
    for (int i = 0; i < str.length(); i++)
    {
        hash_val
            = (hash_val
               + (str[i] - 'a' + 1) * power_of_p)
              % m;
        power_of_p
            = (power_of_p * p) % m;
    }
    return hash_val;
}

unsigned long long int DJBhash(string str)
{
    unsigned long hash = 5381;
    for (auto c : str)
    {
        hash = (hash << 5) + hash + c; /* hash * 33 + c */
    }
    return hash%FILMES_TAM;
}

unsigned long long int double_hash(unsigned long long int pos,unsigned long long int i,unsigned long long int h2,unsigned long long int tam)
{

    return ((pos+i*h2)&0x7FFFFFFF)%tam;
}


//Verifica se a string está na tabela hash
int busca(char *str)
{
    long int index=DJBhash(str);
    long int h2=polynomialRollingHash(str);
    for(int i=0; i<FILMES_TAM; i++)
    {
        int newpos=double_hash(index,i,h2,FILMES_TAM);

        if(hash_table[newpos]!=NULL && hash_table[newpos]->nome.compare(str))
        {
            printf("Achou\n");
            return 1;
        }

        else
        {
            printf("Nao achou\n");
            return -1;

        }

    }
}

//Insere filmes na Hash Table
long long int insereHashTable(filme *c)
{
    if(c==NULL)
        return 0;
    long long int index=DJBhash(c->nome);
    long long int h2=polynomialRollingHash(c->nome);
    int col=0;

    for(int i=0; i<FILMES_TAM; i++)
    {
        long long int newpos=double_hash(index,i,h2,FILMES_TAM);
        if(hash_table[newpos]==NULL)
        {
            hash_table[newpos]=c;

            return 1;
        }


    }

    return 0;
}

//Compara strings
int Comp_str(string s1, string s2)
{

    if (s1 != s2)
    {

        return -1;
    }
    else
    {

        return 1;
    }

}

//retorna posição na tabela hash
int ret(string str)
{
    long int index=DJBhash(str);
    long int h2=polynomialRollingHash(str);
    for(int i=0; i<FILMES_TAM; i++)
    {
        int newpos=double_hash(index,i,h2,FILMES_TAM);

        if(hash_table[newpos]!=NULL && (Comp_str(hash_table[newpos]->nome,str)==1))
        {
            return newpos;
        }

    }
}

//printa tabela hash
void printa_tabela()
{
    for(long long int i=0; i<FILMES_TAM; i++)
    {
        if(hash_table[i]==NULL)
        {
            std::cout<<i<<"\t%lld\t---"<<endl;
        }
        else
        {
            std::cout<<i<< "          "<<hash_table[i]->nome<<endl;
            std::cout<<hash_table[i]->titulo<<endl;
        }

    }
}


// Retorna um novo nodo da Trie (inicializados para NULL)
struct TrieNode *getNode(void)
{
    struct TrieNode *pNode =  new TrieNode;

    pNode->isEndOfWord = false;

    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        pNode->children[i] = NULL;
    }

    return pNode;
}

// Se não está presente, insere chave na Trie. Se a chave é prefixo de nodo na Trie, só marca o nodo folha
void insert(struct TrieNode *root, string key,string id)
{
    struct TrieNode *raizAux = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - ' ';
        if (!raizAux->children[index])
        {
            raizAux->children[index] = getNode();

        }
        raizAux->data=key;



        raizAux = raizAux->children[index];
    }


    // Marca o último nodo como folha

    raizAux->isEndOfWord = true;

}
//Verifica se possui filhos
bool isLastNode(struct TrieNode* root)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (root->children[i])
            return 0;
    return 1;
}


// Função recursiva para printar as sugestões para um dado nodo
void suggestionsRec(struct TrieNode* root, string currPrefix)
{

    // Acha uma string na Trie dado um dado prefixo
    if (root->isEndOfWord)
    {
        prefixos.push_back(currPrefix);

    }

    // Todos ponteiros de nodos da estrutura dos filhos são nulos
    if (isLastNode(root))
        return ;

    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i])
        {
            // Anexa o caracter à atual string
            currPrefix.push_back(32 + i);

            // Recorre sobre o resto
            suggestionsRec(root->children[i], currPrefix);


            // Remove último caracter
            currPrefix.pop_back();

        }
    }

}

// Printa sugestões para úma dada query
int printAutoSuggestions(TrieNode* root, const string query)
{
    struct TrieNode* raizAux = root;

    // Checa se o prefixo está presente no nodo com último caracter de uma dada string
    int level;
    for (level = 0; level < query.length(); level++)
    {
        int index = query[level] - ' ';

        // no string in the Trie has this prefix
        if (!raizAux->children[index])
            return 0;

        raizAux = raizAux->children[index];

    }
    // Se o prefixo está presente na palavra
    bool isWord = (raizAux->isEndOfWord == true);

    // Se o prefixo for o último nodo da árvore (não há filhos)

    bool isLast = isLastNode(raizAux);

    // Se o prefixo é uma palavra, nõa há sub-árvore abaixo do último nodo correspondente

    if (isWord && isLast)
    {
        std::cout << query << endl;
        return -1;
    }

    // Se não há nodos embaixo do último caracter correspondente
    if (!isLast)
    {
        string prefix = query;

        suggestionsRec(raizAux, prefix);

        return 1;
    }
}

//Procura determinada chave na arvore trie
bool search(struct TrieNode *root, string key)
{
    struct TrieNode *raizAux = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - ' ';
        if (!raizAux->children[index])
            return false;

        raizAux = raizAux->children[index];
    }

    return (raizAux != NULL && raizAux->isEndOfWord);
}

//aloca memória 
bool already_allocated = false;
void allocate( int n)
{
    if( already_allocated)
    {
        delete[] names ;
    }
    names = new std::string[n];
    already_allocated = true;
}

//Verifica se o titulo está presente na tabela hash de filmes
int busca_tit(string str)
{

    for(int i=0; i<FILMES_TAM; i++)
    {
        if(hash_table[i]!=NULL)
        {
            if(Comp_str(hash_table[i]->titulo,str)==1)
            {

                vetorpair.push_back(make_pair(i,hash_table[i]->media));
                vetorAux.push_back(hash_table[i]->media);
                return 1;

            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//																			//
//																			//
//							INICIALIZA TABELAS								//							
//																			//
//																			//
//																			//
//////////////////////////////////////////////////////////////////////////////
void inicializa_tabela()
{

    for(long long int i=0; i<FILMES_TAM; i++)
    {

        hash_table[i]=NULL;

    }

}
void inicializa_tabela1()
{

    for(long long int i=0; i<RATING_TAM ; i++)
    {

        hash_table_rat[i]=NULL;

    }

}


//////////////////////////////////////////////////////////////////////////////
//																			//
//																			//
//							FUNÇÕES DE HASH	PARA							//							
//									RATING									//
//																			//
//																			//
//////////////////////////////////////////////////////////////////////////////
unsigned long long int DJBhash1(string str)
{
    unsigned long hash = 5381;
    for (auto c : str)
    {
        hash = (hash << 5) + hash + c;
    }
    return hash%RATING_TAM ;
}

unsigned long long int double_hash1(unsigned long long int pos,unsigned long long int i,unsigned long long int h2,unsigned long long int tam)
{

    return ((pos+i*h2)&0x7FFFFFFF)%tam;
}

long long polynomialRollingHash1(
    string const& str)
{
    int p = 31;
    int m = 1e9 + 9;
    long long power_of_p = 1;
    long long hash_val = 0;

    // Loop para calcular o valor do hash por iteração com outros elementos da string

    for (int i = 0; i < str.length(); i++)
    {
        hash_val = (hash_val+ (str[i] - 'a' + 1) * power_of_p)% m;
        power_of_p= (power_of_p * p) % m;
    }
    return hash_val;
}

//insere na tabela hash de avaliacoes(rating)
long long int insereTag(Avaliacoes *c)
{
    if(c==NULL)
        return 0;
    long long int index=DJBhash1(c->user);
    long long int h2=polynomialRollingHash1(c->user);
    int col=0;

    for(int i=0; i<RATING_TAM ; i++)
    {
        long long int newpos=double_hash1(index,i,h2,RATING_TAM );
        if(hash_table_rat[newpos]!=NULL&& (Comp_str(hash_table_rat[newpos]->user,c->user)==1))
        {
            return 0;
        }
        if(hash_table_rat[newpos]==NULL)
        {
            hash_table_rat[newpos]=c;

            return 1;
        }



    }

    return 0;
}

// Procura rating na tabela Hash e retorna posicao na tabela
int retRating(string str)
{
    long int index=DJBhash1(str);
    long int h2=polynomialRollingHash1(str);
    for(int i=0; i<RATING_TAM ; i++)
    {
        int newpos=double_hash1(index,i,h2,RATING_TAM );

        if(hash_table_rat[newpos]!=NULL && (Comp_str(hash_table_rat[newpos]->user,str)==1))
        {
            return newpos;
        }

    }
}

// Procura rating na tabela Hash
int buscaRating(string str)
{
    long int index=DJBhash1(str);
    long int h2=polynomialRollingHash1(str);
    for(int i=0; i<RATING_TAM ; i++)
    {
        int newpos=double_hash1(index,i,h2,RATING_TAM );
        int p;


        if(hash_table_rat[newpos]!=NULL  && (Comp_str(hash_table_rat[newpos]->user,str) ==1))
        {
            std::cout<<hash_table_rat[newpos]->user;
            printf("achou\n");
            return 1;
        }
    }
    printf("nao achou\n");
    return -1;
}






//Guarda no vetor o par da posição e da lista de generos
vector<pair<int,string>>buscaGen(string str)
{
    vector<pair<int,string>>aux;
    int l=0;

    for(int i=0; i<FILMES_TAM; i++)
    {
        if(hash_table[i]!=NULL)
        {



            if((hash_table[i]->lista.find(str)!= std::string::npos) &&hash_table[i]->num_ava>=1000)
            {
                aux.push_back(make_pair(i,hash_table[i]->lista));

            }
            l++;



        }
    }
    return aux;
    std::cout<<l<<endl;
}

//Insere na tabela hash de tags
long long int insereTag1(tag *c)
{

    if(c==NULL)
        return 0;
    long long int index=DJBhash(c->tag);
    long long int h2=polynomialRollingHash(c->tag);
    int col=0;

    for(int i=0; i< RATING_TAM; i++)
    {
        long long int newpos=double_hash(index, i,h2,  RATING_TAM);
        if(hash_table_tag[newpos]!=NULL&& (Comp_str(hash_table_tag[newpos]->tag,c->tag)==1))
        {
            return 0;
        }
        if(hash_table_tag[newpos]==NULL)
        {
            hash_table_tag[newpos]=c;

            return 1;
        }

    }

    return 0;
}

//printa tabela hash de tags
void printa_tabela2()
{
    for(long long int i=0; i< RATING_TAM; i++)
    {
        if(hash_table_tag[i]==NULL)
        {
            std::cout<<i<<"\t%lld\t---"<<endl;
        }
        else
        {
            std::cout<<i<<"\t"<<hash_table_tag[i]->tag<<endl;
            for(int j=0; j<hash_table_tag[i]->movieId.size(); j++)
                std::cout<<hash_table_tag[i]->movieId[j]<<endl;

        }

    }
}

// Procura tag na tabela Hash
int retTag(string str)
{
    long int index=DJBhash(str);
    long int h2=polynomialRollingHash(str);
    int newpos;
    for(int i=0; i< RATING_TAM; i++)
    {
        newpos=double_hash(index, i,h2, RATING_TAM);

        if(hash_table_tag[newpos]!=NULL && (Comp_str(hash_table_tag[newpos]->tag,str)==1))
        {
            return newpos;
        }

    }
}

//////////////////////////////////////////////////////////////////////////////
//																			//
//																			//
//							Queries do Programa								//							
//																			//
//																			//
//																			//
//////////////////////////////////////////////////////////////////////////////


//dada uma entrada com as tags retorna os filmes que possuem essas determinadas tags
void query4(vector<string>teste_tag)
{
    for(int i=1; i<teste_tag.size(); i++)
    {

        teste_tag[i].erase(0, 1);
        teste_tag[i].erase(teste_tag[i].size() - 1);
    }


    vector<string>teste2;
    vector<string>dentro;
    for(int i=0; i<RATING_TAM; i++)
    {
        if(hash_table_tag[i]!=NULL)
        {
            for(int k=1; k<teste_tag.size(); k++)
            {
                if(hash_table_tag[i]->tag == teste_tag[k])
                {
                    for(int j=0; j<hash_table_tag[i]->movieId.size(); j++)
                    {

                        teste2.push_back(hash_table_tag[i]->movieId[j]);
                    }
                }
            }
        }
    }
    std::cout<<"comeco verificacao \n";
    for(int i=0; i<teste2.size(); i++)
    {
        int cnt;
        cnt= count(teste2.begin(), teste2.end(), teste2[i]);
        if(cnt==teste_tag.size()-1)
        {


            if(!(std::find(dentro.begin(), dentro.end(), teste2[i]) != dentro.end()))
            {
                dentro.push_back(teste2[i]);

                int p=ret(teste2[i]);
                std::cout<<"title:"<<hash_table[p]->titulo;
                std::cout<<"    genres:"<<hash_table[p]->lista;
                std::cout<<"    rating:"<<hash_table[p]->media;
                std::cout<<"    count:"<<hash_table[p]->num_ava<<endl<<endl;

            }
        }
    }
}

//Retorna informações de todos filmes avaliados por um determinado usuário
void query2(string entrada)
{
    int h;
    h=retRating(entrada);

    for(int i=0; i<hash_table_rat[h]->notas.size(); i++)
    {
        std::cout<<"User Rating:  "<< hash_table_rat[h]->notas[i];
        int k;
        k=ret(hash_table_rat[h]->filmes[i]);
        std::cout<<"           title:    "<<hash_table[k]->titulo;
        std::cout<<"           Global Rating:     "<<hash_table[k]->media;
        std::cout<<"           Count:      "<<hash_table[k]->num_ava;
        std::cout<<"\n\n";

    }

}

// retorna top N filmes de um determinado genero de entrada, ordenado por rating
void query3(string entrada,int num_entrada)
{

    entrada.erase(0, 1);
    entrada.erase(entrada.size() - 1);
    for(auto& c : entrada)
    {
        c = tolower(c);
    }
    entrada[0] = toupper(entrada[0]);


    vector<pair<int,string>>teste;
    vector<pair<int, float>>teste3;

    std::cout<<"ok"<<endl;
    int indice_gen=0;
    teste=buscaGen(entrada);

    for(int i=0; i<teste.size(); i++)
    {

        indice_gen=teste[i].first;
        teste3.push_back(make_pair(teste[i].first,hash_table[indice_gen]->media));

    }

    std::sort(teste3.begin(), teste3.end(), sort_pair_second<float, float, std::greater<float> >());
    int indice2=0;
    int num=10;
    for(int i=0; i<num_entrada; i++)
    {
        indice2=teste3[i].first;
        std::cout<<"   title:  "<<hash_table[indice2]->titulo;
        std::cout<<"   genres:  "<<hash_table[indice2]->lista;
        std::cout<<"   rating:   "<<hash_table[indice2]->num_ava;
        std::cout<<"   count:   "<<teste3[i].second<<endl<<endl;


    }
}


//Dada um string retorna os filmes que contém a string como prefixo


void query1Aux(TrieNode *root, string str)
{
    printAutoSuggestions(root,str);

    for(int i=0; i<prefixos.size(); i++)
    {
        busca_tit(prefixos[i]);

    }
}

void query1(string entrada, TrieNode *root)
{

    query1Aux(root,entrada);
    std::sort(vetorAux.begin(),vetorAux.end(),greater<float>());
    int indice_tab;
    for(int i=0; i<vetorAux.size(); i++)
    {
        for(int p = 0; p < vetorpair.size(); p++)
        {
            if(vetorpair[p].second==vetorAux[i])
            {
                indice_tab=vetorpair[p].first;
                std::cout<<"Movieid:   "<<hash_table[indice_tab]->nome<<"  title:   "<<hash_table[indice_tab]->titulo;
                std::cout<<"   genres:        "<<hash_table[indice_tab]->lista<<"    rating:   " << vetorpair[p].second<<"   count:  "<<hash_table[indice_tab]->num_ava<<endl<<endl;

            }

        }

    }
}

//Split string em um vetor 
vector<string> split(string str, string token)
{
    vector<string>resultado;
    while(str.size())
    {
        int index = str.find(token);
        if(index!=string::npos)
        {
            resultado.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)resultado.push_back(str);
        }
        else
        {
            resultado.push_back(str);
            str = "";
        }
    }
    return resultado;
}

int main()
{
	//inicializa tabelas
    inicializa_tabela();
    inicializa_tabela1();
	
	//começa contagem tempo
    float tempo;
    time_t ini,fim;
    ini=time(NULL);
	
	//inicializa tabelas aux para hash
    filme *tabela[FILMES_TAM];
    for(long int i =0; i<FILMES_TAM; i++)
        tabela[i] =  new(nothrow)  filme;

    Avaliacoes *tabela1[RATING_TAM];
    for(long int i =0; i<RATING_TAM; i++)
    {
        tabela1[i] =  new(nothrow)  Avaliacoes;

    }
	
	//inicializa trie tree
    struct TrieNode *root = getNode();

	// abre arquivos movie e rating
    std::ifstream f("movie_clean.csv");
    std::ifstream r("rating2.csv");
	
	//Inicializa os parsers movie e rating
    CsvParser parser(f);
    CsvParser parser2(r);

	//strings aux
    string str;
	string strs;
	
	//indices /variaveis aux
    int i=0;
    int j=0;
    int p=0;
    int t=0;
	int k=0;
	int l=0;
	
	//aloca espaço
    allocate(40000);
    string *ids = new string[150000];
    string *lista=new string[150000];
    string *media=new string;

  
  
   //Faz o parse do arquivo movie para estruturas necessárias
    for (auto& row : parser)
    {

        for (const std::string & field  : row)
        {
            if(i==0)
            {
                k++;
                if(k<FILMES_TAM)
                {
                    tabela[k]->nome=field;
                    insereHashTable(tabela[k]);
                    p=ret(field);

                }

            }


            if(i==1)
            {
                if(k<FILMES_TAM)
                {
                    hash_table[p]->titulo=field;
                    names[j]=field;
                    insert(root,names[j],ids[j]);

                }

            }
            if(i==2)
            {
                hash_table[p]->lista=field;
                lista[j]=field;
                ids[j]=field;

            }

            i++;
        }
        j++;
        i=0;

    }
    int a=0;
    for(i=0; i<FILMES_TAM; i++)
    {
        if(hash_table[i]!=NULL)
        {
            hash_table[i]->media=0;
            hash_table[i]->num_ava=0;
        }
    }
    i=0;
    j=0;
	
	//Faz o parse do arquivo rating para estruturas necessárias
    for (auto& row1 : parser2)
    {
        for (auto& field1 : row1)
        {



            if(i==0)// Pega primeiro campo(tag)
            {


                if(str!=field1)
                {


                    tabela1[j]->user=field1;
                    insereTag(tabela1[j]);
                    t=retRating(field1);
                    j++;

                    if(j%10000==0)
                    {
                        std::cout<<j<<endl;
                    }

                }
                str=field1;


            }

            if(i==1)// Pega segundo campo (movieId) // como associar movieId com a os usuarios na tabela
            {
                hash_table_rat[t]->filmes.push_back(field1);
                p=ret(field1);

            }

            if(i==2)// Pega segundo campo(Rating)
            {
                strs=field1;
                hash_table_rat[t]->notas.push_back(field1);

                hash_table[p]->media+=std::atof(strs.c_str());

                hash_table[p]->num_ava++;

            }
            i++;
        }
        i=0;
    }



    for(i=0; i<FILMES_TAM; i++)
    {
        if(hash_table[i]!=NULL)
        {
            if(hash_table[i]->media !=0)
            {
                if(hash_table[i]->nome!="movieId")
                {
                    hash_table[i]->media/=hash_table[i]->num_ava;
                }
            }
        }
	
    }
	
    vector<tag>tabela2(467733);
	
	
    std::ifstream y("tag_clean.csv");
    CsvParser parser3(y);
    k=0;
    string straux;
	
	//faz o parse do arquivo tag para estruturas necessárias
    for (auto& row2 : parser3)
    {
        for (auto& field : row2)
        {
            k++;

            if(i==0)// Pega primeiro campo(tag)
            {

            }


            if(i==1)// Pega segundo campo (movieId)
            {
                straux=field;


            }
            if(i==2)// Pega segundo campo(Rating)
            {

                tabela2[l].tag=field;

                insereTag1(&tabela2[l]);
                p= retTag(field);
                if (!(std::find(hash_table_tag[p]->movieId.begin(), hash_table_tag[p]->movieId.end(), straux) != hash_table_tag[p]->movieId.end()))
                    hash_table_tag[p]->movieId.push_back(straux);
                l++;

            }
            i++;
        }
        i=0;
    }
	
    string string_resposta;
	
	//enquanto usuario desejar queries
    while(string_resposta!="SAIR")
    {
		//pega  a entrada
        std::cout<<"Digite a query que deseja testar: "<<endl;
        string entr;
        getline(cin,entr);

		//vetores aux
        vector<string>entrada_user;
        vector<string>num_tag;
        entrada_user=split(entr," ");
        vector<string>teste2;
        vector<string>dentro;
		
		
        std::locale loc;
        int num_para_tag;
		
		//converte lower case
        for(auto& c : entrada_user[0])
        {
            c = tolower(c);
        }

		//faz a comparação da entrada
		//querie tag
        if((entrada_user[0])=="tags")
            query4(entrada_user);
        else
        {
			//querie movie
            if(entrada_user[0]=="movie")
            {
                string auxiliar;
                auxiliar=entr;
                auxiliar=auxiliar.substr(auxiliar.find_first_of(" \t")+1);

                query1(auxiliar,root);

            }
            else
            {
				//querie user
                if(entrada_user[0]=="user")
                    query2(entrada_user[1]);
                else
                {
					//querie top N
                    if(entrada_user[0].find("top") != std::string::npos)
                    {
                        num_tag=split(entrada_user[0],"p");

                        num_para_tag=atoi(num_tag[1].c_str());

                        query3(entrada_user[1],num_para_tag);


                    }
                }

            }

        }
        std::cout<<"Digite qualquer coisa para fazer uma nova query ou SAIR para encerrar o programa"<<endl;
        getline(cin,string_resposta);
        for(auto& c : string_resposta)
        {
            c = toupper(c);
        }
        system("CLS");

    }
	//encerra e calcula tempo
    printf("Programa Encerrado\n");
    
    fim=time(NULL);
    std::cout<<difftime(fim,ini);

    return 0;
}
