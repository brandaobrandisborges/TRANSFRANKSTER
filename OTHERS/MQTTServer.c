/*
* Includes
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>

/*
* Defines
*/
/* Caso desejar utilizar outro broker MQTT, substitua o endereco abaixo */
#define MQTT_ADDRESS   "test.mosquitto.org"
/* Substitua este por um ID unico em sua aplicacao */
#define CLIENTID       "MQTTClientServerFRANK"  

/* Substitua aqui os topicos de publish e subscribe por topicos exclusivos de sua aplicacao */
#define MQTT_PUBLISH_TOPIC     "FRANKSTEINASDA_SUB"
//#define MQTT_PUBLISH_TOPIC_DISP   "FRANKSTEINASDA_SUB"
//A subscricao com # define todos subtopicos do mesmo... Vai ser responsavel para armazenar os PEERS/FILES
#define MQTT_PUBLISH_TOPIC_DISP "unifei/redes/FRANKSTEIN/HUB/AVALIABLE/#"
#define MQTT_PUBLISH_TOPIC_REQ    "unifei/redes/FRANKSTEIN/HUB/REQUEST/#"

/*
*  Variaveis globais
*/
MQTTClient client;

//TABELA DE PEER/TABLE
char **peer_table = NULL;
char **file_table = NULL;
int table_count = 0;

/*
* Prototipos de funcao
*/
void publish(MQTTClient client, char* topic, char* payload);
int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message);

/*
* Implementacoes
*/

/* Funcao: publicacao de mensagens MQTT
 * Parametros: cleinte MQTT, topico MQTT and payload
 * Retorno: nenhum
*/
void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 2;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
}

/* Funcao: callback de mensagens MQTT recebidas e echo para o broker
 * Parametros: contexto, ponteiro para nome do topico da mensagem recebida, tamanho do nome do topico e mensagem recebida
 * Retorno : 1: sucesso (fixo / nao ha checagem de erro neste exemplo)
*/
int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payload = message->payload;
    /* Mostra a mensagem recebida */
    //printf("Mensagem recebida! \n\rTopico: %s Mensagem: %s\n", topicName, payload);

    //SE A MENSAGEM FOR NO TOPIC AVALIABLE, DEVE SER INSERIDA O PEER/FILE.
    //CASO CONTRARIO, TRATA-SE DE UMA REQUISICAO DE ARQUIVO...
    

    if(strstr(topicName, "AVALIABLE")!=NULL){
        char * ip = NULL;
        ip = malloc (15);
        if(ip == NULL){
            printf("Erro de memoria\n");
            return 0;
        }
        
        ip = topicName+38;
        printf("Recebido disponibilidade de [%s] para [%s]\n", ip, payload);
        
        if(peer_table == NULL){
        //CRIA TABELA DE PEER
        table_count++;
        peer_table = malloc (table_count * sizeof(char*));
        *(peer_table + table_count) = ip;
        

        //CRIA TABELA DE FILE
        file_table = malloc (table_count * sizeof(char*));
        *(file_table + table_count) = payload;
               
        printf("Criado... Vamos imprimir\n");
        printf("| %s | ---- | %s |", *(file_table + table_count), *(peer_table + table_count));

        }else{
            //TABELA JA EXISTE, SIMPLESMENTE ADICIONA NOVO VALOR
            table_count++;
            
            peer_table = realloc (peer_table, table_count * sizeof(char*));
            *(peer_table + table_count) = ip;

            file_table = realloc (file_table, table_count * sizeof(char*));
            *(file_table + table_count) = payload;

            printf("Realocado... Vamos imprimir\n");
            printf("| %s | ---- | %s |", *(file_table + table_count), *(peer_table + table_count));
        }
    }else{
        printf("Requisicao de arquivo\n");

        int k;
        int len = strlen(*file_table);
        for (k = 0; k < table_count; k++){
            printf("| %s | ---- | %s | --- %d", *(file_table + k), *(peer_table + k), len);
        }
    }
    
    /* Faz echo da mensagem recebida */
    //publish(client, MQTT_PUBLISH_TOPIC, payload);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main(int argc, char *argv[])
{
   int rc;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

   /* Inicializacao do MQTT (conexao & subscribe) */
   MQTTClient_create(&client, MQTT_ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);

   rc = MQTTClient_connect(client, &conn_opts);

   if (rc != MQTTCLIENT_SUCCESS)
   {
       printf("\n\rFalha na conexao ao broker MQTT. Erro: %d\n", rc);
       exit(-1);
   }

   MQTTClient_subscribe(client, MQTT_PUBLISH_TOPIC_DISP, 0);
   MQTTClient_subscribe(client, MQTT_PUBLISH_TOPIC_REQ, 0);

   while(1)
   {
       printf("LOOPING>\n");
       usleep(500000);

       /*
        * o exemplo opera por "interrupcao" no callback de recepcao de 
        * mensagens MQTT. Portanto, neste laco principal eh preciso fazer
        * nada.
        */
   }
}