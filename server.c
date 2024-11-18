#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>

#define DEFAULT_BUFFER_SIZE 4096
#define LOG_FILE "server_log.txt"
#define STATS_COUNT 20

static int server_socket;

// Simple structure for transfer statistics
struct transfer_stats
{
	double time;
	double rate;
	size_t size;
};

static struct transfer_stats stats_history[STATS_COUNT];
static int stats_count = 0;

static void handle_signal(int sig)
{
	printf("\nReceived signal %d, shutting down...\n", sig);

	// Log final statistics
	FILE *log_file = fopen(LOG_FILE, "a");
	if (log_file != NULL)
	{
		time_t current_time;
		time(&current_time);
		fprintf(log_file, "\n=== Server Shutdown ===\n");
		fprintf(log_file, "Time: %s", ctime(&current_time));
		log_transfer_summary(log_file);
		fclose(log_file);
	}

	close(server_socket);
	exit(0);
}

// Add transfer statistics to history
void add_transfer_stat(double transfer_time, double rate, size_t size)
{
	if (stats_count == STATS_COUNT)
	{
		memmove(&stats_history[0], &stats_history[1],
				sizeof(struct transfer_stats) * (STATS_COUNT - 1));
		stats_count--;
	}
	stats_history[stats_count].time = transfer_time;
	stats_history[stats_count].rate = rate;
	stats_history[stats_count].size = size;
	stats_count++;
}

// Log transfer summary statistics
void log_transfer_summary(FILE *log_file)
{
	if (stats_count == 0)
		return;

	double min_time = stats_history[0].time;
	double max_time = stats_history[0].time;
	double total_time = 0;
	double min_rate = stats_history[0].rate;
	double max_rate = stats_history[0].rate;
	double total_rate = 0;

	for (int i = 0; i < stats_count; i++)
	{
		// Update min/max time
		if (stats_history[i].time < min_time)
			min_time = stats_history[i].time;
		if (stats_history[i].time > max_time)
			max_time = stats_history[i].time;
		total_time += stats_history[i].time;

		// Update min/max rate
		if (stats_history[i].rate < min_rate)
			min_rate = stats_history[i].rate;
		if (stats_history[i].rate > max_rate)
			max_rate = stats_history[i].rate;
		total_rate += stats_history[i].rate;
	}

	fprintf(log_file, "\n=== Transfer Statistics Summary ===\n");
	fprintf(log_file, "Number of transfers: %d\n", stats_count);
	fprintf(log_file, "Transfer Time (seconds):\n");
	fprintf(log_file, "  Min: %.6f\n", min_time);
	fprintf(log_file, "  Max: %.6f\n", max_time);
	fprintf(log_file, "  Avg: %.6f\n", total_time / stats_count);
	fprintf(log_file, "Transfer Rate (KB/s):\n");
	fprintf(log_file, "  Min: %.2f\n", min_rate);
	fprintf(log_file, "  Max: %.2f\n", max_rate);
	fprintf(log_file, "  Avg: %.2f\n", total_rate / stats_count);
}

// Get unique filename by appending (2), (3), etc.
char *get_unique_filename(const char *original)
{
	char base[256], ext[32] = "", temp[512];
	const char *dot = strrchr(original, '.');

	if (dot)
	{
		int base_len = dot - original;
		strncpy(base, original, base_len);
		base[base_len] = '\0';
		strcpy(ext, dot);
	}
	else
	{
		strcpy(base, original);
	}

	// Try original filename first
	if (access(original, F_OK) != 0)
	{
		return strdup(original);
	}

	// Try with numbers
	for (int i = 2; i < 100; i++)
	{
		snprintf(temp, sizeof(temp), "%s(%d)%s", base, i, ext);
		if (access(temp, F_OK) != 0)
		{
			return strdup(temp);
		}
	}
	return NULL;
}

// Log transfer statistics (renamed parameter from 'time' to 'transfer_time')
void log_stats(FILE *log_file, double transfer_time, double rate,
			   const char *filename, size_t size, const char *saved_as)
{
	time_t current_time;
	time(&current_time);

	fprintf(log_file, "\n=== Transfer Complete ===\n");
	fprintf(log_file, "Time: %s", ctime(&current_time));
	fprintf(log_file, "Original filename: %s\n", filename);
	fprintf(log_file, "Saved as: %s\n", saved_as);
	fprintf(log_file, "Size: %lu bytes\n", size);
	fprintf(log_file, "Transfer time: %.6f seconds\n", transfer_time);
	fprintf(log_file, "Transfer rate: %.2f KB/s\n", rate);
}

int main(int argc, char *argv[])
{
	// Check command line arguments
	if (argc < 2 || argc > 3)
	{
		printf("Usage: %s port-number [bufSize]\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);
	int buffer_size = (argc == 3) ? atoi(argv[2]) : DEFAULT_BUFFER_SIZE;

	if (port <= 0 || buffer_size <= 0)
	{
		printf("Invalid port or buffer size\n");
		return 1;
	}

	// Set up signal handling
	struct sigaction sa = {0};
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Create and configure socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		perror("Could not create socket");
		return 1;
	}

	// Allow socket reuse
	int flag = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	// Bind socket
	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind failed");
		close(server_socket);
		return 1;
	}

	// Listen for connections
	if (listen(server_socket, 1) < 0)
	{
		perror("Listen failed");
		close(server_socket);
		return 1;
	}

	printf("Server listening on port %d\n", port);
	// After socket setup and before the main loop:
	FILE *log_file = fopen(LOG_FILE, "a");
	if (log_file != NULL)
	{
		time_t current_time;
		time(&current_time);
		fprintf(log_file, "\n=== Server Started ===\n");
		fprintf(log_file, "Time: %s", ctime(&current_time));
		fprintf(log_file, "Port: %d\n", port);
		fprintf(log_file, "Buffer Size: %d bytes\n", buffer_size);
		fclose(log_file);
	}

	// Main server loop
	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);

		printf("Waiting for connections...\n");
		int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
		if (client_socket < 0)
		{
			perror("Accept failed");
			continue;
		}

		printf("Connection from %s:%d\n",
			   inet_ntoa(client_addr.sin_addr),
			   ntohs(client_addr.sin_port));

		// Receive filename length and filename
		uint32_t filename_length;
		recv(client_socket, &filename_length, sizeof(filename_length), 0);
		filename_length = ntohl(filename_length);

		char filename[256];
		recv(client_socket, filename, filename_length, 0);
		filename[filename_length] = '\0';

		// Receive file size
		uint64_t file_size;
		recv(client_socket, &file_size, sizeof(file_size), 0);
		file_size = be64toh(file_size);

		printf("Receiving file: %s (size: %lu bytes)\n", filename, file_size);

		// Get unique filename and open file
		char *unique_filename = get_unique_filename(filename);
		if (!unique_filename)
		{
			fprintf(stderr, "Could not create unique filename\n");
			close(client_socket);
			continue;
		}

		FILE *file = fopen(unique_filename, "wb");
		if (!file)
		{
			perror("Error opening file");
			free(unique_filename);
			close(client_socket);
			continue;
		}

		// Start timing
		struct timespec start_time, end_time;
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		// Receive file data
		char buffer[buffer_size];
		uint64_t total_received = 0;
		int transfer_success = 1;

		while (total_received < file_size)
		{
			size_t to_receive = (file_size - total_received < buffer_size) ? file_size - total_received : buffer_size;

			ssize_t received = recv(client_socket, buffer, to_receive, 0);
			if (received <= 0)
			{
				transfer_success = 0;
				break;
			}

			if (fwrite(buffer, 1, received, file) != received)
			{
				transfer_success = 0;
				break;
			}

			total_received += received;
			printf("\rProgress: %.1f%%", (total_received * 100.0) / file_size);
			fflush(stdout);
		}

		fclose(file);

		// Calculate transfer statistics
		if (transfer_success && total_received == file_size)
		{
			clock_gettime(CLOCK_MONOTONIC, &end_time);
			double transfer_time =
				(end_time.tv_sec - start_time.tv_sec) +
				(end_time.tv_nsec - start_time.tv_nsec) / 1e9;

			if (transfer_time < 0.000001)
				transfer_time = 0.000001;
			double transfer_rate = (file_size / 1024.0) / transfer_time;

			// After transfer completion:
			printf("\nTransfer %s!\n", transfer_success ? "complete" : "failed");
			if (transfer_success)
			{
				printf("Saved as: %s\n", unique_filename);
				printf("Transfer time: %.6f seconds\n", transfer_time);
				printf("Transfer rate: %.2f KB/s\n", transfer_rate);
			}

			// Add to statistics
			add_transfer_stat(transfer_time, transfer_rate, file_size);

			// Log transfer
			FILE *log_file = fopen(LOG_FILE, "a");
			if (log_file)
			{
				log_stats(log_file, transfer_time, transfer_rate,
						  filename, file_size, unique_filename);

				// If we've collected STATS_COUNT transfers, log summary
				if (stats_count == STATS_COUNT)
				{
					log_transfer_summary(log_file);
					stats_count = 0; // Reset counter after logging summary
				}
				fclose(log_file);
			}
		}
		else
		{
			printf("\nTransfer failed!\n");
			remove(unique_filename);
		}

		free(unique_filename);
		close(client_socket);
	}

	close(server_socket);
	return 0;
}