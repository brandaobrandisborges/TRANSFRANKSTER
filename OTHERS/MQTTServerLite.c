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
//FILA PARA OS DE ARQUIVO DISPONIVEL OUVIREM
#define MQTT_PUBLISH_TOPIC_REQ_RESP    "unifei/redes/FRANKSTEIN/HUB/REQ_RESP/"

/*
*  Variaveis globais
*/
MQTTClient client;

//TABELA DE PEER/TABLE
char *peer_table[100];
char *file_table[100];
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
        
        if(table_count <= 99){
            peer_table[table_count] = NULL;
            peer_table[table_count] = malloc (100);
            if(peer_table[table_count]==NULL){
                printf("Falha no malloc. Returnando\n");
                return 0;
            }

            file_table[table_count] = "";
            file_table[table_count] = malloc (100);
            if(file_table[table_count]==NULL){
                printf("Falha no malloc. Returnando\n");
                return 0;
            }

            //CRIA TABELA DE PEER
            strcpy(peer_table[table_count], ip);
            //CRIA TABELA DE FILE
            strcpy(file_table[table_count], payload);
            //file_table[table_count] = payload;

            printf("Criado...\n");
            printf("| %s | ---- | %s |", file_table[table_count], peer_table[table_count]);
            table_count++;
        }else{
            printf("Server overload. Try latter\n");
            return 0;
        }
    }else{
        char * ipx = NULL;
        ipx = malloc (15);
        if(ipx == NULL){
            printf("Erro de memoria\n");
            return 0;
        }
        
        ipx = topicName+36;
        printf("Requisicao de arquivo. Temos:\n");

        int k;
        for (k = 0; k < table_count; k++){
            printf("| %s | ---- | %s |\n", file_table[k], peer_table[k]);
            if(strcmp(file_table[k], payload)==0){
                printf("Found match. Sending MQTT to start transaction. Wont stop to initiate more peers.\n");
                char *topic_temp = NULL;
                topic_temp = malloc (225);

                if(topic_temp == NULL){
                    printf("Falha ao alocar. Tente novamente\n");
                    return 0;
                }
                //DEFINE SE EH REQUISITO OU DISPONIBILIZACAO
                strcpy(topic_temp, MQTT_PUBLISH_TOPIC_REQ_RESP);
                strcat(topic_temp, peer_table[k]);

                char *pay_req = NULL;
                pay_req = malloc (225);

                if(pay_req == NULL){
                    printf("Falha ao alocar. Tente novamente\n");
                    return 0;
                }
                //DEFINE SE EH REQUISITO OU DISPONIBILIZACAO
                strcpy(pay_req, file_table[k]);
                strcat(pay_req, "|||");
                strcat(pay_req, ipx);

                printf("Requesting IP [%s] to send data [%s] to IP [%s] through topic [%s]. Pay load request is [%s]...", peer_table[k], file_table[k], ipx, topic_temp, pay_req);
                publish(client, topic_temp, pay_req);
            }

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