#include <stdio.h>
#include <iostream>
#include <new>
#include <string.h>
#include <chrono>
#include <filesystem>
using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;// i had to update c++ version to use this

//after consideration with ai about performance we decided to change any int -> size_t 

size_t Buffer_Size = 1024 * 1024; // 1MB buffer size for reading files (default)
char CodeTable[256][256];

size_t g_totalBytes = 0;
size_t g_processedBytes = 0;
chrono::high_resolution_clock::time_point g_startTime;
char bar[11] = "0000000000"; // Progress bar representation
size_t g_totalBytes_decomp = 0;
size_t g_processedBytes_decomp = 0;
chrono::high_resolution_clock::time_point g_startTime_decomp;


struct FrequencyNode {
	unsigned char byte;
	size_t frequency;
	FrequencyNode* left;
	FrequencyNode* right;
	FrequencyNode* next;// to use as linked-list;
	FrequencyNode() {
		left = right = next = nullptr;
	}
};

class Huffman {
	FrequencyNode* head;
	FrequencyNode* root;
	char* filename;
	bool isLeaf(FrequencyNode* node) {
		if (node == nullptr)
			return false;
		if (node->left == nullptr && node->right == nullptr)
			return true;
		return false;
	}
	void Generate_Code_table(FrequencyNode* recNode, char CodeTable[256][256], char* code, size_t depth) {
		if (recNode == nullptr)
			return;
		if (isLeaf(recNode)) {
			code[depth] = '\0';
			for (size_t i = 0; i < depth; i++) {
				CodeTable[recNode->byte][i] = code[i];//after each leaf node we write the code to the table
			}
			return;
		}
		//left subtree
		code[depth] = '0';
		Generate_Code_table(recNode->left, CodeTable, code, depth + 1);

		//right subtree
		code[depth] = '1';
		Generate_Code_table(recNode->right, CodeTable, code, depth + 1);
	}
public:
	Huffman() {
		head = root = nullptr;
	}
	void insert(unsigned char a) {
		if (head == nullptr) {
			head = new FrequencyNode();
			head->byte = a;
			head->frequency = 1;
			return;
		}

		FrequencyNode* tmp = head;
		FrequencyNode* prev = nullptr;
		while (tmp) {
			if (tmp->byte == a) {
				tmp->frequency++;
				return;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		FrequencyNode* newNode = new FrequencyNode();
		newNode->byte = a;
		newNode->frequency = 1;
		prev->next = newNode;
	}
	FrequencyNode* build_frequency_table(char* file_name) {//add the bytes or increment frequency
		this->filename = file_name;
		FILE* file;
		fopen_s(&file, filename, "rb");
		if (!file) {
			cout << "Error opening file!" << endl;
			return nullptr;
		}
		unsigned char* Buffer = new(nothrow)unsigned char[Buffer_Size];//chatgpt
		if (!Buffer) {
			cout << "Memory allocation failed!" << endl;
			fclose(file);
			return nullptr;
		}
		size_t bytesRead;//chatgpt


		while ((bytesRead = fread(Buffer, sizeof(unsigned char), Buffer_Size, file)) > 0) {//this logic is not ours, chatgpt made it for us
			g_processedBytes += bytesRead;
			auto now = chrono::high_resolution_clock::now();
			double elapsed = chrono::duration_cast<chrono::milliseconds>(now - g_startTime).count() / 1000.0;
			double percent = (double)g_processedBytes / g_totalBytes * 100.0;
			double eta = (g_processedBytes > 0) ? (elapsed * (g_totalBytes - g_processedBytes) / g_processedBytes) : 0.0;
			switch ((int)(percent / 10) - 1) {
			case 0: strcpy(bar, "=#########"); break;
			case 1: strcpy(bar, "==########"); break;
			case 2: strcpy(bar, "===#######"); break;
			case 3: strcpy(bar, "====######"); break;
			case 4: strcpy(bar, "=====#####"); break;
			case 5: strcpy(bar, "======####"); break;
			case 6: strcpy(bar, "=======###"); break;
			case 7: strcpy(bar, "========##"); break;
			case 8: strcpy(bar, "=========#"); break;
			case 9: strcpy(bar, "##########"); break;
			default: strcpy(bar, "##########"); break;
			}
			printf("\rProgress: [%s] %.2f%%, ETA: %.2f seconds", bar, percent, eta);
			fflush(stdout);
			for (size_t i = 0; i < bytesRead; i++) {
				insert(Buffer[i]);
			}
		}
		delete[] Buffer;
		fclose(file);
		return head;
	}
	void print_list() {
		FrequencyNode* tmp = head;
		while (tmp) {
			cout << tmp->byte << " " << tmp->frequency << endl;
			tmp = tmp->next;
		}
	}
	void delete_list() {
		FrequencyNode* tmp = head;
		while (tmp) {
			FrequencyNode* next = tmp->next;
			delete tmp;
			tmp = next;
		}
		head = nullptr;
	}
	void delete_tree(FrequencyNode* node) {
		if (node != nullptr) {
			delete_tree(node->left);
			delete_tree(node->right);
			delete node;
		}
	}
	~Huffman() {
		//delete_list();
		delete_tree(root);
	}
	void sort_list() {
		if (head == nullptr || head->next == nullptr)
			return;
		bool swapped;
		do {
			swapped = false;
			FrequencyNode* current = head;
			FrequencyNode* prev = nullptr;
			while (current != nullptr && current->next != nullptr) {
				if (current->frequency > current->next->frequency) {
					swapped = true;
					FrequencyNode* next = current->next;
					current->next = next->next;
					next->next = current;
					if (prev == nullptr) {
						head = next;
					}
					else {
						prev->next = next;
					}
					prev = next;
				}
				else {
					prev = current;
					current = current->next;
				}
			}
		} while (swapped);
	}
	void build_tree() {//the huffman tree
		while (head && head->next) {//at least 2 nodes
			FrequencyNode* first = head;
			FrequencyNode* second = head->next;
			head = second->next;
			FrequencyNode* combined = new FrequencyNode();//huffman node
			combined->left = first;
			combined->right = second;
			combined->frequency = first->frequency + second->frequency;
			combined->byte = NULL;
			combined->next = head;
			head = combined;
			sort_list();
		}
		root = head;
	}
	void pub_Generate_code_table(char CodeTable[256][256], char* code, size_t depth) {//public version of byte-code converter
		Generate_Code_table(root, CodeTable, code, depth);
	}
	void compress(char* filename, char* outFilename, char codetable[256][256]) {
		FILE* input;
		FILE* output;
		fopen_s(&input, filename, "rb");
		if (!input) {
			cout << "Error opening file!" << endl;
			return;
		}

		strcat(outFilename, ".ece2103");//add .ece2103 extension to the output file name
		fopen_s(&output, outFilename, "wb");
		if (!output) {
			cout << "Error opening output file!" << endl;
			fclose(input);
			return;
		}
		unsigned char* Buffer = new(nothrow) unsigned char[Buffer_Size];
		if (!Buffer) {
			cout << "Memory allocation failed!" << endl;
			fclose(input);
			fclose(output);
			return;
		}
		size_t bytesRead;

		unsigned char outByte = 0;
		size_t outBitCount = 0;
		while ((bytesRead = fread(Buffer, sizeof(unsigned char), Buffer_Size, input)) > 0) {
			g_processedBytes += bytesRead;
			auto now = chrono::high_resolution_clock::now();
			double elapsed = chrono::duration_cast<chrono::milliseconds>(now - g_startTime).count() / 1000.0;
			double percent = (double)g_processedBytes / g_totalBytes * 100.0;
			double eta = (g_processedBytes > 0) ? (elapsed * (g_totalBytes - g_processedBytes) / g_processedBytes) : 0.0;
			switch ((int)(percent / 10) - 1) {
			case 0: strcpy(bar, "=#########"); break;
			case 1: strcpy(bar, "==########"); break;
			case 2: strcpy(bar, "===#######"); break;
			case 3: strcpy(bar, "====######"); break;
			case 4: strcpy(bar, "=====#####"); break;
			case 5: strcpy(bar, "======####"); break;
			case 6: strcpy(bar, "=======###"); break;
			case 7: strcpy(bar, "========##"); break;
			case 8: strcpy(bar, "=========#"); break;
			case 9: strcpy(bar, "##########"); break;
			default: strcpy(bar, "##########"); break;
			}
			printf("\rProgress: [%s] %.2f%%, ETA: %.2f seconds", bar, percent, eta);
			fflush(stdout);
			for (size_t i = 0; i < bytesRead; i++) {
				unsigned char byte = Buffer[i];
				char* code = codetable[byte];
				size_t j = 0;
				while (code[j] != '\0') {
					outByte = outByte << 1;//bitwise op. 0110->01100 that shifts bits to left with 0
					if (code[j] == '1') {
						outByte = outByte | 1;//bitwise op. 01100 makes or operation with last bit in byte
						//                  1
					}//                                     01101
					outBitCount++;
					if (outBitCount == 8) {
						fwrite(&outByte, 1, 1, output);
						outByte = 0;
						outBitCount = 0;
					}
					j++;
				}
			}
		}
		// Write any remaining bits (pad with zeros)
		if (outBitCount > 0) {
			outByte = outByte << (8 - outBitCount);
			fwrite(&outByte, 1, 1, output);
		}
		delete[] Buffer;
		fclose(input);
		fclose(output);
	}
	void generate_cod_file(char* fileName, char* originalFileName) {
		char codFilename[512];
		strcpy(codFilename, fileName);
		strcat(codFilename, ".cod");
		FILE* file;
		fopen_s(&file, codFilename, "w");
		if (!file) {
			cout << "Error opening file!" << endl;
			return;
		}
		fprintf(file, "%s\n", originalFileName);
		for (size_t i = 0; i < 256; i++) {
			if (CodeTable[i][0] != '\0') {
				fprintf(file, "%d %s\n", i, CodeTable[i]);
			}
		}
		fclose(file);
	}
	void reconstruct_tree_from_cod(const char* codFileName, char* outFilename) {
		FILE* codFile;
		fopen_s(&codFile, codFileName, "r");
		if (!codFile) {
			cout << "Error opening .cod file!" << endl;
			outFilename[0] = '\0';
			return;
		}

		// Read the first line (filename)
		if (fgets(outFilename, 256, codFile) == nullptr) {
			cout << "Error reading filename from .cod file!" << endl;
			fclose(codFile);
			outFilename[0] = '\0';
			return;
		}
		// Remove newline character if present
		size_t len = strlen(outFilename);
		if (len > 0 && outFilename[len - 1] == '\n') {
			outFilename[len - 1] = '\0';
		}
		// Create a new root node
		FrequencyNode* newRoot = new FrequencyNode();
		size_t byteValue;
		char code[256];

		while (fscanf(codFile, "%d %s", &byteValue, code, (unsigned)_countof(code)) == 2) {
			FrequencyNode* current = newRoot;
			for (size_t i = 0; code[i] != '\0'; i++) {
				if (code[i] == '0') {
					if (!current->left) {
						current->left = new FrequencyNode();
					}
					current = current->left;
				}
				else if (code[i] == '1') {
					if (!current->right) {
						current->right = new FrequencyNode();
					}
					current = current->right;
				}
			}
			// At leaf node, store the byte value
			current->byte = static_cast<unsigned char>(byteValue);
		}

		fclose(codFile);
		root = newRoot;
	}

	void decompress(char* inputFileName, char* outputFileName)
	{
		if (!root) {//check if the tree is empty
			cout << "Error: Huffman tree is empty!" << endl;
			return;
		}

		FILE* input;
		FILE* output;
		fopen_s(&input, inputFileName, "rb");
		fopen_s(&output, outputFileName, "wb");

		if (!input || !output) {
			cout << "Error opening input or output file!" << endl;
			if (input) fclose(input);
			if (output) fclose(output);
			return;
		}

		FrequencyNode* current = root;
		unsigned char* Buffer = new(nothrow) unsigned char[Buffer_Size];
		if (!Buffer) {
			cout << "Memory allocation failed!" << endl;
			fclose(input);
			fclose(output);
			return;
		}
		unsigned char* OutputBuffer = new(nothrow) unsigned char[Buffer_Size];
		if (!OutputBuffer) {
			cout << "Memory allocation failed!" << endl;
			delete[] Buffer;
			fclose(input);
			fclose(output);
			return;
		}
		size_t outputIndex = 0;

		g_totalBytes_decomp = fs::file_size(inputFileName);
		g_processedBytes_decomp = 0;
		g_startTime_decomp = chrono::high_resolution_clock::now();

		size_t bytesRead;
		while ((bytesRead = fread(Buffer, sizeof(unsigned char), Buffer_Size, input)) > 0) {//read from input file
			g_processedBytes_decomp += bytesRead;
			auto now = chrono::high_resolution_clock::now();
			double elapsed = chrono::duration_cast<chrono::milliseconds>(now - g_startTime_decomp).count() / 1000.0;
			double percent = (double)g_processedBytes_decomp / g_totalBytes_decomp * 100.0;
			double eta = (g_processedBytes_decomp > 0) ? (elapsed * (g_totalBytes_decomp - g_processedBytes_decomp) / g_processedBytes_decomp) : 0.0;
			switch ((int)(percent / 10) - 1) {
			case 0: strcpy(bar, "=#########"); break;
			case 1: strcpy(bar, "==########"); break;
			case 2: strcpy(bar, "===#######"); break;
			case 3: strcpy(bar, "====######"); break;
			case 4: strcpy(bar, "=====#####"); break;
			case 5: strcpy(bar, "======####"); break;
			case 6: strcpy(bar, "=======###"); break;
			case 7: strcpy(bar, "========##"); break;
			case 8: strcpy(bar, "=========#"); break;
			case 9: strcpy(bar, "##########"); break;
			default: strcpy(bar, "##########"); break;
			}
			printf("\rProgress: [%s] %.2f%%, ETA: %.2f seconds", bar, percent, eta);
			fflush(stdout);
			for (size_t b = 0; b < bytesRead; b++) {//iterate through each byte in the buffer
				unsigned char inByte = Buffer[b];
				for (int i = 7; i >= 0; i--) {//by the first look you might say this is wrong, but the 8 bits are just the way of getting info from buffer
					int bit = (inByte >> i) & 1;//e.x 01101101 --> 00000000 and 1 --> 00000000 so 0
					if (bit == 0) current = current->left;//if bit is 0 go left
					else current = current->right;//if bit is 1 go right

					if (isLeaf(current)) {
						OutputBuffer[outputIndex++] = current->byte;//if we are at leaf node, we write the byte to output buffer	
						current = root;
						if (outputIndex == Buffer_Size) {
							fwrite(OutputBuffer, 1, Buffer_Size, output);//write the output buffer to file
							outputIndex = 0;//reset output index
						}
					}
				}
			}
		}
		// Write any remaining bytes in the output buffer
		if (outputIndex > 0) {
			fwrite(OutputBuffer, 1, outputIndex, output);
		}

		delete[] Buffer;//delete buffer input
		delete[] OutputBuffer;//delete buffer output
		fclose(input);
		fclose(output);
	}

};

int main(int argc, char* args[]) {//app.exe h psdfnls -r
	for (int i = 0; i < argc; i++) {
		printf("Argument [%d]: %s\n", i, args[i]);
	}
	char filename[] = " microprocessor_sheet2.pdf";
	char compressed_filename[] = "compressed.ece2103";//default compressed file name
	char output_filename[256] = "decompressed.bmp";
	bool compress = false, decompress = false;
	if (argc < 2) {
		cout << "Error in Arguments !!" << endl;
		return 0;
	}
	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "-c") == 0 && i + 1 < argc) {
			strcpy(filename, args[i + 1]);
			strcpy(output_filename, args[i + 2]);
			compress = true;
			i++;
		}
		if (strcmp(args[i], "-d") == 0 && i + 1 < argc) {
			strcpy(compressed_filename, args[i + 1]);
			strcpy(output_filename, args[i + 2]);
			decompress = true;
			i++;
		}
		if (strcmp(args[i], "-b") == 0 && i + 1 < argc) {
			Buffer_Size = atoi(args[i + 1]);
			Buffer_Size *= 1024;//turn into KB
			i++;
		}
		if (strcmp(args[i], "--help") == 0) {
			cout << "Usage: " << args[0] << " -c <filename> | -d <compressed_filename> [-b <buffer_size_in_kb>]" << endl;
			cout << "Options:" << endl;
			cout << "  -c <filename> <output name(without extension)>      Compress the specified file." << endl;
			cout << "  -d <compressed_filename> <output name(without extension)> Decompress the specified compressed file." << endl;
			cout << "  -b <buffer_size_in_kb> Set the buffer size for reading files (default is 1MB)." << endl;
			cout << "  --help              Show this help message." << endl;
			return 0;
		}
	}


	Huffman* huffman = new Huffman();
	if (compress) {
		size_t fileSize = fs::file_size(filename);
		g_totalBytes = fileSize * 2; // both phases
		g_processedBytes = 0;
		g_startTime = chrono::high_resolution_clock::now();
		huffman->build_frequency_table(filename);
		//huffman->print_list();
		//cout << "Sorting the list..." << endl;
		huffman->sort_list();
		//huffman->print_list();
		//cout << "Building the Huffman tree..." << endl;
		huffman->build_tree();
		//cout << "Huffman Tree built successfully!" << endl;
		//huffman->print_list();

		//cout << "Generating code table..." << endl;
		char code[256];
		int depth = 0;
		huffman->pub_Generate_code_table(CodeTable, code, depth);
		//cout << "Code table generated successfully!" << endl;
		//for (int i = 0; i < 256; i++) {
		//	//if (CodeTable[i][0] != '\0') {
		//	cout << "Byte: " << i << " Code: " << CodeTable[i] << endl;
		//	//}
		//}
		//cout << "Compressing the file..." << endl;


		huffman->compress(filename, output_filename, CodeTable);		//cout << "File compressed successfully!" << endl;//file.fsd->file.fsd.ece2103,file.fsd.cod->file.fsd
		char cod_base[256];
		huffman->generate_cod_file(output_filename, filename);
		//cout << "Code file generated successfully!" << endl;
	}


	if (decompress) {
		char cod_filename[256];
		strcpy(cod_filename, compressed_filename);
		strcat(cod_filename, ".cod");
		char original_name[256];//empty
		huffman->reconstruct_tree_from_cod(cod_filename, original_name);//this gets original name
		char* out_ext = strrchr(output_filename, '.');
		if (out_ext) {
			*out_ext = '\0';
		}
		// Append the original extension from the cod file
		char* ext = strrchr(original_name, '.');
		if (ext) {
			strcat(output_filename, ext);
		}
		huffman->decompress(compressed_filename, output_filename);
		//cout << "File decompressed successfully!" << endl;

	}
	delete huffman;
	return 0;
}

//10-->1100010
//11-->1100011
//12-->1101000