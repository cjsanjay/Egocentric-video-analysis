#include "processvideo.h"

typedef struct  WAV_HEADER {
	char                RIFF[4];        // RIFF Header      Magic header
	unsigned long       ChunkSize;      // RIFF Chunk Size  
	char                WAVE[4];        // WAVE Header      
	char                fmt[4];         // FMT header       
	unsigned long       Subchunk1Size;  // Size of the fmt chunk                                
	unsigned short      AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	unsigned short      NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
	unsigned long       SamplesPerSec;  // Sampling Frequency in Hz                             
	unsigned long       bytesPerSec;    // bytes per second 
	unsigned short      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
	unsigned short      bitsPerSample;  // Number of bits per sample      
	char                Subchunk2ID[4]; // "data"  string   
	unsigned long       Subchunk2Size;  // Sampled data length    

}wav_hdr;



processvideo::processvideo()
{
}

processvideo::processvideo(string s1,int h1,int w1)
{
	framepath = s1;
	h = h1;
	w = w1;
	yframes = vector<double **>(2, NULL);
	remod = new double*[h];
	for (int l = 0; l<h; l++) {
		remod[l] = new double[w];
	}
	final1 = new double*[8];
	for (int q = 0; q<8; q++) {
		final1[q] = new double[8];
	}
	temp = new double*[8];
	for (int q = 0; q<8; q++) {
		temp[q] = new double[8];
	}
	ybuf1 = new double*[h];
	for (int i = 0; i<h; i++) {
		ybuf1[i] = new double[w];
	}

	ybuf2 = new double*[h];
	for (int i = 0; i<h; i++) {
		ybuf2[i] = new double[w];
	}
}


/*
Function to find size of file
*/
int processvideo::getFileSize() // path to file
{		
	FILE *p_file = fopen(framepath.c_str(), "rb");	
	fseek(p_file, 0, SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;
}

// find the file size 
int getFileSize2(FILE *inFile) {
	int fileSize = 0;
	fseek(inFile, 0, SEEK_END);

	fileSize = ftell(inFile);

	fseek(inFile, 0, SEEK_SET);
	return fileSize;
}



string getFileName(string name) {
	int i = name.size() - 1;
	while (name[i] != '.') {
		i = i - 1;
	}
	int end = i - 1;
	i = 0;
	string result = "";
	for (int i = 0; i <= end; i++) {
		result += name[i];
	}
	return result;
}

/*
Function to find differenc in video frames and after reconstructing the frames
*/
void processvideo::processFrame2() {
	double **data_prev = yframes[0];
	int count = 0;
	
	double **data = yframes[1];
	for (int j = 0; j<h; j = j + 8) {
		for (int k = 0; k<w; k = k + 8) {
			count = reconstructFrame(data_prev, data, remod, j, k, count);
		}
	}	
	result_frame_diff.push_back(frameDiff(remod, data));
}

/*
Function to reconstruct the current frame using previous frame
*/
int processvideo::reconstructFrame(double ** prev, double **curr, double **remod, int row, int col, int count) {
	int sub1 = 8;
	while (row - sub1<0) {
		sub1 = sub1 - 1;
	}
	int sub2 = 8;
	while (col - sub2<0) {
		sub2 = sub2 - 1;
	}
	int row_lim = row + 16;
	while (row_lim >= h) {
		row_lim = row_lim - 1;
	}
	int col_lim = col + 16;
	while (col_lim >= w) {
		col_lim = col_lim - 1;
	}
	
	float final_min = 32767;
	float temp_min = 100;
	
	int count1 = 0;
	for (int i = row - sub1; i <= row_lim - 8; i = i + 8) {
		for (int j = col - sub2; j <= col_lim - 8; j = j + 8) {
			for (int k = i, r_s = 0; k<i + 8; k++, r_s++) {
				for (int m = j, c_s = 0; m<j + 8; m++, c_s++) {
					temp[r_s][c_s] = prev[k][m];
				}
			}
			count++;
			count1++;
			temp_min = getSmallBlockDiff(curr, temp, row, col);
			if (temp_min<final_min) {
				final_min = temp_min;
				for (int z = 0; z<8; z++) {
					for (int y = 0; y<8; y++) {
						final1[z][y] = temp[z][y];
					}
				}
			}
		}
	}
	row_lim = row + 8;
	if (row_lim >= h) {
		row_lim = h;
	}
	col_lim = col + 8;
	if (col_lim >= w) {
		col_lim = w;
	}
	for (int i = row, m = 0; i<row_lim; i++, m++) {
		for (int j = col, k = 0; j<col_lim; j++, k++) {
			remod[i][j] = final1[m][k];
		}
	}	
	return count;
}


/*
Funtion to read video frame from video file and convert to YUV color space and store and return Y channel
*/
void processvideo::readFrames2(FILE* IN_FILE, int index,char** Rbuf, char** Gbuf, char **Bbuf,double **ybuf) {	
	int i;
	for (i = 0; i < h; i++)
	{
		for (int j = 0; j<w; j++) {
			Rbuf[i][j] = fgetc(IN_FILE);
		}
	}
	for (i = 0; i < h; i++)
	{
		for (int j = 0; j<w; j++) {
			Gbuf[i][j] = fgetc(IN_FILE);
		}
	}
	for (i = 0; i < h; i++)
	{
		for (int j = 0; j<w; j++) {
			Bbuf[i][j] = fgetc(IN_FILE);
		}
	}
	for (i = 0; i < h; i++)
	{
		for (int j = 0; j<w; j++) {
			ybuf[i][j] = 0.299f * (Rbuf[i][j] & 0xff) + 0.587f * (Gbuf[i][j] & 0xff) + 0.114f * (Bbuf[i][j] & 0xff);
		}
	}
	yframes[index] = ybuf;	
}


/*
Read output frame from input file and write that frame in output file
*/
void processvideo::read_write(FILE* IN_FILE, FILE* OUT_FILE, char** Rbuf, char** Gbuf, char **Bbuf,vector<vector<int> > result_frames) {
	int i;
	int frame_offset = h*w * 3;
	for (int m = 0; m < result_frames.size(); m++) {
		cout << result_frames[m][0] << endl;
		fseek(IN_FILE, frame_offset*result_frames[m][0]*15, SEEK_SET);
		for (int k = (result_frames[m][0] * 15); k <= result_frames[m][1] * 15; k++) {
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j<w; j++) {
					Rbuf[i][j] = fgetc(IN_FILE);
				}
			}
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j<w; j++) {
					Gbuf[i][j] = fgetc(IN_FILE);
				}
			}
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j<w; j++) {
					Bbuf[i][j] = fgetc(IN_FILE);
				}
			}
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++) {
					fputc(Rbuf[i][j], OUT_FILE);
				}
			}
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++) {
					fputc(Gbuf[i][j], OUT_FILE);
				}
			}
			for (i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++) {
					fputc(Bbuf[i][j], OUT_FILE);
				}
			}
		}
	}
		
}

/*
Read output frame from input file and write that frame in output file
*/
int processvideo::read_write_a(string f_name, vector<vector<int> > result_frames) {
	wav_hdr wavHeader;
	FILE *wavFile;
	string out_f_name = getFileName(f_name);
	out_f_name += "_out.wav";
	wavFile = fopen(f_name.c_str(), "r");	
	wavFile = fopen(f_name.c_str(), "r");
	int headerSize = sizeof(wav_hdr), filelength = 0;
	if (wavFile == NULL) {
		printf("Can not able to open wave file\n");		
	}
	fread(&wavHeader, headerSize, 1, wavFile);
	fclose(wavFile);

	char *memblock, *memblock1;
	int num_of_bytes = wavHeader.bytesPerSec;	//any user specific number of bytes
	memblock = new char[num_of_bytes];
	memblock1 = new char[44];

	//source file
	ifstream src(f_name.c_str(), ios::in | ios::binary);

	//dest file
	ofstream dest(out_f_name.c_str(), ios::out | ios::binary);

	//if we couldn't open the source file (it may not exist), means fail
	if (!src.is_open())
	{
		cout << "Failed to open the file.\n";
		return 0;
	}
	//reading "num_of_bytes" byte(s) from sourcefile
	src.read(memblock1, 44);

	//counting number of bytes actually read
	//using gcount() function
	num_of_bytes = src.gcount();

	//writing "num_of_bytes" byte(s) to destination file
	dest.write(memblock1, num_of_bytes);
	int count = 0;
	num_of_bytes = wavHeader.bytesPerSec;
	int audio_offset = wavHeader.bytesPerSec;
	for (int i = 0; i < result_frames.size(); i++) {
		int start = result_frames[i][0];
		int end = result_frames[i][1];
		src.seekg(audio_offset*start, src.beg);
		while (start <= end) {
			src.read(memblock, num_of_bytes);

			//counting number of bytes actually read
			//using gcount() function
			num_of_bytes = src.gcount();

			//if zero bytes read, means End Of File
			//has reached, no bytes left to read
			if (num_of_bytes == 0)	break;

			//writing "num_of_bytes" byte(s) to destination file
			dest.write(memblock, num_of_bytes);
			start = start + 1;
		}
	}

	src.close();
	dest.close();
}

/*
Calculate difference between two frames 
*/
double processvideo::frameDiff(double **data_prev, double **data) {
	double diff = 0;
	double frameCoeff = 0;
	for (int i = 0; i<h; i++) {
		for (int j = 0; j<w; j++) {
			diff += fabs(data_prev[i][j] - data[i][j]);
			frameCoeff += data[i][j];
		}
	}
	avg_frame_color.push_back(frameCoeff/ (h*w));
	return diff/(h*w);
}


/*
Get Difference between two 8x8 blocks in a video frame
*/
float processvideo::getSmallBlockDiff(double **curr, double **temp, int row, int col) {
	float diff = 0;
	int row_lim = row + 8;
	if (row_lim >= h) {
		row_lim = h;
	}
	int col_lim = col + 8;
	if (col_lim >= w) {
		col_lim = w;
	}
	for (int i = row, m = 0; i<row_lim; i++, m++) {
		for (int j = col, k = 0; j<col_lim; j++, k++) {
			diff += fabs(curr[i][j] - temp[m][k]);
		}
	}
	return diff;
}

void processvideo::clearLastFrame()
{
	delete yframes[0];
}

void processvideo::setLastFrame()
{
	yframes[0]=yframes[1];
}

string processvideo::getPath() {
	return framepath;
}

double** processvideo::getFirst() {
	return ybuf1;
}

double** processvideo::getSecond() {
	return ybuf2;
}


/*
Find relevant sequence of scenes in a sequence of continuous scene
*/
void find_scene(int start, int end,vector<vector<int> > &this_is_final,map<int,double> result_avg) {
	int count2 = 0;
	double sum2 = 0;
	vector<int> scene;
	for (int i = start; i <= end; i++) {
			count2++;
			sum2 += result_avg[i];
			if (count2 == 10) {
				scene.push_back(sum2 / 10);
				sum2 = 0;
				count2 = 0;
			}			
	}
	int current = scene[0];
	int temp_start=start;
	int temp_end = start+10;
	for (int i = 1; i < scene.size(); i++) {
		if (fabs(current - scene[i])<=5) {
			temp_start += 10;
			temp_end += 10;
		}
		else {
			vector<int> temp;
			temp.push_back(temp_start);
			temp.push_back(temp_end);
			this_is_final.push_back(temp);
			current = scene[i];
			temp_start += 10;
			temp_end += 10;
		}
	}
	vector<int> temp;
	temp.push_back(temp_start);
	temp.push_back(temp_end);
	this_is_final.push_back(temp);
}

vector<vector<int>> processvideo::evaluateResult() {
	int frame_count = 1;
	int size = result_frame_diff.size();
	double sum = 0;

	//counting frame difference value's count in frame_color
	for (int i = 0; i < size; i++) {
		int hash_num_seconds = frame_count / 15;
		if ((frame_count+1) % 15 != 0) {
			sum += avg_frame_color[i];
		}else {
			sum += avg_frame_color[i];
			sum /= 15;			
			avg_frame_color_sec[hash_num_seconds]=sum;
			sum = 0;
		}	
		frame_count++;
	}

	int start = -1;
	int end = -1;
	frame_count = 1;
	/*
	Find continous scene in group of frames
	*/
	vector<vector<int>> temp_data;
	map<int, int> valid_range;
	for (int i = 1; i < size; i++) {
		int num_seconds = (frame_count / 15);
		int minute = (frame_count / 15) / 60;
		valid_range[int(fabs(result_frame_diff[i] - result_frame_diff[i - 1]))]++;
		if (fabs(result_frame_diff[i] - result_frame_diff[i - 1]) <= 6) {
			if (start == -1) {
				start = num_seconds;
			}			
		}
		else {
			if (start != -1) {
				end =num_seconds;			
				temp_data.push_back(vector<int>(1, start));
				int sz = temp_data.size()-1;
				temp_data[sz].push_back(end);
				start = -1;
				end = -1;
			}			
		}
		frame_count++;
	}

	start = -1;
	end = -1;

	// Find continous group of frames, for which start and end frame are not more apart then 2
	vector<vector<int>> valid_frame_group;
	for (int i = 0; i < temp_data.size(); i++) {
		if (temp_data[i][1] - temp_data[i][0]>2 || temp_data[i][1] - temp_data[i][0] == 0) {
			if (start == -1) {
				start = temp_data[i][0];
				end = temp_data[i][1];
			}else {
				if (end == temp_data[i][0]) {
					end = temp_data[i][1];
				}else {
					if (start != end && end-start>=10) {
						valid_frame_group.push_back(vector<int>(1, start));
						int sz = valid_frame_group.size() - 1;
						valid_frame_group[sz].push_back(end);
					}					
					start = temp_data[i][0];
					end = temp_data[i][1];
				}
			}			
		}	
	}
	vector<vector<int> > summarized_frames;

	
	for (int i = 0; i < valid_frame_group.size(); i++) {
		if (valid_frame_group[i][1] - valid_frame_group[i][0] <= 10) {
			vector<int> temp;
			temp.push_back(valid_frame_group[i][0]);
			temp.push_back(valid_frame_group[i][1]);
			summarized_frames.push_back(temp);
		}
		else {
			find_scene(valid_frame_group[i][0], valid_frame_group[i][1], summarized_frames, avg_frame_color_sec);
		}
	}
	return summarized_frames;
}



int h = 270;
int w = 480;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cout << "Usage: processvideo video_file.rgb audio_file.wav" << endl;
		return 0;
	}
	string input_file_v = argv[1];
	string input_file_a = argv[2];
	FILE *IN_FILE = NULL;
	IN_FILE=fopen(input_file_v.c_str(), "rb");
	processvideo p1(input_file_v.c_str(), h,w);
	int file_length = p1.getFileSize();
	int number_of_frame = file_length / (w*h * 3);	
	int current_frame = 1;
	char **Rbuf = new char*[h];
	char **Gbuf = new char*[h];
	char **Bbuf = new char*[h];	
	for (int i = 0; i<h; i++) {
		Rbuf[i] = new char[w];	
		Gbuf[i] = new char[w];
		Bbuf[i] = new char[w];		
	}

	p1.readFrames2(IN_FILE, 0,Rbuf,Gbuf,Bbuf,p1.getFirst());
	int flag = 1;
	while (current_frame<number_of_frame) {
		if(flag){
			p1.readFrames2(IN_FILE, 1, Rbuf, Gbuf, Bbuf, p1.getSecond());
			flag = 0;
		}else{
			p1.readFrames2(IN_FILE, 1, Rbuf, Gbuf, Bbuf, p1.getFirst());
			flag = 1;
		}
		
		p1.processFrame2();
		p1.setLastFrame();
		current_frame = current_frame + 1;		
		cout << "Processes Frame: " << current_frame << endl;
	}
	fclose(IN_FILE);
	vector<vector<int>> result_frames;
	result_frames=p1.evaluateResult();
	IN_FILE = fopen(input_file_v.c_str(), "rb");
	FILE *OUT_FILE = NULL;	
	string ofname = getFileName(input_file_v);
	ofname += "_out.rgb";
	OUT_FILE = fopen(ofname.c_str(),"wb");
	p1.read_write(IN_FILE, OUT_FILE, Rbuf, Gbuf, Bbuf, result_frames);
	p1.read_write_a(input_file_a,result_frames);
	fclose(IN_FILE);
	fclose(OUT_FILE);
	string temp = getFileName(input_file_a);
	temp += "_out.wav";
	cout << "Output files: " << ofname << " " << temp << endl;
	system("pause");
	return 0;
}

processvideo::~processvideo()
{
}
