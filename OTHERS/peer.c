// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <MQTTClient.h>

//PORTAS UDP
#define PORT	 8080 
#define MAXLINE 1024 

//MQTT PARA CONTROLE PEERS
#define MQTT_ADDRESS   "test.mosquitto.org"
#define CLIENTID       "MQTTCClientDPEER"
//ESTE TOPICO ATINGE O SERVER PRINCIPAL DE CONTROLE
#define MQTT_PUBLISH_TOPIC_DISP     "unifei/redes/FRANKSTEIN/HUB/AVALIABLE/"
#define MQTT_PUBLISH_TOPIC_REQ    "unifei/redes/FRANKSTEIN/HUB/REQUEST/"
#define MQTT_PUBLISH_TOPIC_REQ_RESP    "unifei/redes/FRANKSTEIN/HUB/REQ_RESP/"

MQTTClient client;

void create_connect(int subscribe, char * topic, char* payload);

//MQTT PUBLICA E CALLBACK DE RECEBIMENT MSG
void publish(MQTTClient client, char* topic, char* payload);
int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message);

//PUBLICA DISPONIBILIDADE DE ARQUIVO (MQTT)
int define_arquivo(char * self_ip, char * arquivo, int requisitar);

// Driver code 
int main() { 
	int menu_option;
	char * self_ip = NULL;
	char * listen_req_res = NULL;
	
	printf("\nStarting...\n");

	while(1){
		printf("1 - Configurar IP\n");
		printf("2 - Definir arquivo disponivel\n");
		printf("3 - Requisitar arquivo\n");
		printf("4 - Ouvir para enviar arquivo\n");
		scanf("%d", &menu_option);

		printf("Switch is [%d]\n", menu_option);
		switch(menu_option){
			case 1:
				self_ip = malloc(100);
				printf("Entre com o IP formato decimal: ");
				scanf("%s", self_ip);
				printf("Definido IP [%s]\n", self_ip);
			break;
			
			case 2:
				if(self_ip == NULL)
					printf("IP nao corretamente configurado. Entre com opcao 1 primeiro.\n");
				else if(strlen(self_ip)<=0)
					printf("IP nao corretamente configurado. Entre com opcao 1 primeiro.\n");
				else{
					char *arquivo_disp = malloc(100);
					printf("Entre com o nome do ARQUIVO disponivel: ");
					scanf("%s", arquivo_disp);
					printf("Definindo ARQUIVO [%s]. Aguarde.\n", arquivo_disp);
					define_arquivo(self_ip, arquivo_disp, 0);
					free(arquivo_disp);
				}
			break;

			case 3:
				if(self_ip == NULL)
					printf("IP nao corretamente configurado. Entre com opcao 1 primeiro.\n");
				else if(strlen(self_ip)<=0)
					printf("IP nao corretamente configurado. Entre com opcao 1 primeiro.\n");
				else{
					char *arquivo_req = malloc(100);
					printf("Entre com o nome do ARQUIVO requisitado: ");
					scanf("%s", arquivo_req);
					printf("Definindo ARQUIVO [%s]. Aguarde.\n", arquivo_req);
					define_arquivo(self_ip, arquivo_req, 1);
					free(arquivo_req);
				}
			break;

			case 4:
				listen_req_res = malloc (170);
				if(listen_req_res == NULL){
					printf("Erro de memoria\n");
					return 0;
				}
				
				strcpy(listen_req_res, MQTT_PUBLISH_TOPIC_REQ_RESP);
                strcat(listen_req_res, self_ip);
				
				printf("Ouvindo TOPIC [%s]\n", listen_req_res);
				create_connect(1, listen_req_res, NULL);
			break;

			case 9:
				printf("Bye!\n");
				break;
			
		}
	}

	int sockfd; 
	char buffer[MAXLINE]; 
	char *hello = "Hello from client"; 
	struct sockaddr_in servaddr; 

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	
	int n, len; 
	
	sendto(sockfd, (const char *)hello, strlen(hello), 
		MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
			sizeof(servaddr)); 
	printf("Hello message sent.\n"); 
		
	n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
				MSG_WAITALL, (struct sockaddr *) &servaddr, 
				&len); 
	buffer[n] = '\0'; 
	printf("Server : %s\n", buffer); 

	close(sockfd); 
	return 0; 
} 

//DEFINE ARQUIVO DISPONIVEL PELO PEER.
int define_arquivo(char * self_ip, char * arquivo_nome, int requisitar){
	char *topic_temp = NULL;
	topic_temp = malloc (225);

	if(topic_temp == NULL){
		printf("Falha ao alocar. Tente novamente\n");
		return 0;
	}
	//DEFINE SE EH REQUISITO OU DISPONIBILIZACAO
	strcpy(topic_temp, requisitar == 1 ? MQTT_PUBLISH_TOPIC_REQ : MQTT_PUBLISH_TOPIC_DISP);
	strcat(topic_temp, self_ip);

	printf("Enviando MQTT para [%s] definir ARQUIVO [%s] disponivel...\n", topic_temp, arquivo_nome);
	create_connect(0, topic_temp, arquivo_nome);
	return 1;
	
	
}

void create_connect(int subscribe, char * topic, char* payload){
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_create(&client, MQTT_ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);
	int rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS)
		printf("\n\rFalha na conexao ao broker MQTT. Erro: %d\n", rc);
	

	//apenas se inscreve e entra em loop caso marcado como true. Para enviar disponibilidade, nao precisa se inscrever.
	if(subscribe==1){
		MQTTClient_subscribe(client, topic, 0);
		while(1){
			usleep(500000);
			printf("Listening...\n");
			//loopa a conexao para oubir o
		}
	}else{//deseja publicar
 		publish(client, topic, payload);
	}

}


void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 2;//GARANTE ENTREGA UNICA DO PAYLOAD
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payload = message->payload;

    /* Mostra a mensagem recebida */
    printf("Mensagem recebida! \n\rTopico: %s Mensagem: %s\n", topicName, payload);

    /* Faz echo da mensagem recebida */
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}