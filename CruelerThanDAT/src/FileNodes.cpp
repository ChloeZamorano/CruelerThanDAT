#include "pch.hpp"
#include "FileNodes.h"

const FbxSystemUnit fbxsdk::FbxSystemUnit::m(100.0);

FileNode* FileNode::selectedNode = nullptr;

std::vector<std::string> BXMInternal::SplitString(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	std::string current;

	for (char c : str) {
		if (c == delimiter) {
			if (!current.empty()) {
				result.push_back(current);
				current.clear();
			}
		}
		else {
			current += c;
		}
	}
	if (!current.empty()) {
		result.push_back(current);
	}

	return result;
}

void closeNode(FileNode* target) {
	FileNode::selectedNode = nullptr;
	auto it = std::find(openFiles.begin(), openFiles.end(), target);
	if (it != openFiles.end()) { // Ensure it exists before erasing
		openFiles.erase(it);
	}

	delete target;
}

FileNode* HelperFunction::LoadNode(std::string fileName, const std::vector<char>& data, bool forceEndianess , bool bigEndian) {
	std::string fileExtension = fileName.substr(fileName.find_last_of("."));
	uint32_t fileType = 0;
	FileNode* outputFile = nullptr;

	// fix extension compare
	size_t nullPos = fileExtension.find('\0');
	if (nullPos != std::string::npos) {
		fileExtension = fileExtension.substr(0, nullPos);
	}

	if (data.size() >= 4) {
		fileType = *reinterpret_cast<const uint32_t*>(&data[0]);
	}



	// TODO: DAT files smaller than 4 bytes (empty) aren't recognized as DAT files
	if (fileExtension == ".uid") {
		outputFile = new UidFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileExtension == ".uvd") {
		outputFile = new UvdFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 5521732) {
		outputFile = new DatFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 1145588546) {
		outputFile = new BnkFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile(); 
	}
	else if (fileType == 876760407) {
		outputFile = new WmbFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 5391187) {
		outputFile = new ScrFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 7630701) {
		outputFile = new MotFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 5000536 || fileType == 5068866) {
		outputFile = new BxmFileNode(fileName);
		outputFile->fileIsBigEndian = false;			
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 3299660) {
		outputFile = new LY2FileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 1481001298) {
		outputFile = new WemFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		outputFile->LoadFile();
	}
	else if (fileType == 4346967) {
		outputFile = new WtbFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
		//outputFile->LoadFile();
	}
	else {
		outputFile = new UnkFileNode(fileName);
		if (forceEndianess) {
			outputFile->fileIsBigEndian = bigEndian;
		}
		outputFile->SetFileData(data);
	}

	return outputFile;
}

int HelperFunction::Align(int value, int alignment) {
	return (value + (alignment - 1)) & ~(alignment - 1);
}

float HelperFunction::HalfToFloat(uint16_t h) {
	uint16_t h_exp = (h & 0x7C00) >> 10;  // exponent
	uint16_t h_sig = h & 0x03FF;          // significand
	uint32_t f_sgn = (h & 0x8000) << 16;  // sign bit

	uint32_t f_exp, f_sig;

	if (h_exp == 0) {
		// Zero / Subnormal
		if (h_sig == 0) {
			f_exp = 0;
			f_sig = 0;
		}
		else {
			// Normalize it
			h_exp = 1;
			while ((h_sig & 0x0400) == 0) {
				h_sig <<= 1;
				h_exp--;
			}
			h_sig &= 0x03FF;
			f_exp = (127 - 15 + h_exp) << 23;
			f_sig = h_sig << 13;
		}
	}
	else if (h_exp == 0x1F) {
		// Inf / NaN
		f_exp = 0xFF << 23;
		f_sig = h_sig << 13;
	}
	else {
		f_exp = (h_exp - 15 + 127) << 23;
		f_sig = h_sig << 13;
	}

	uint32_t f = f_sgn | f_exp | f_sig;
	float result;
	std::memcpy(&result, &f, sizeof(result));
	return result;
}

bool HelperFunction::WriteVectorToFile(const std::vector<char> dataVec, const std::string& filename) {
	std::string fullPath = filename;

	std::ofstream out(fullPath, std::ios::binary);
	if (!out) return false;

	out.write(dataVec.data(), dataVec.size());
	return out.good();
}

int IntLength(int value) {
	int length = 0;
	while (value > 0) {
		value >>= 1;
		length++;
	}
	return length;
}

FileNode::FileNode(std::string fName) {
	fileName = fName;
	fileExtension = fileName.substr(fileName.find_last_of(".") + 1);
}

FileNode::~FileNode() {

}

void FileNode::PopupOptionsEx() {

}

void FileNode::ExportFile() {
	OPENFILENAME ofn;
	wchar_t szFile[260] = { 0 };
	LPWSTR pFile = szFile;

	mbstowcs_s(0, pFile, fileName.length() + 1, fileName.c_str(), _TRUNCATE);

	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lpstrFile = pFile;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.nMaxFile = 260;
	ofn.lpstrFilter = fileFilter;

	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn) == TRUE) {
		std::ofstream outputFile(ofn.lpstrFile, std::ios::binary); // Open in binary mode!
		if (outputFile.is_open()) {
			SaveFile();
			outputFile.write(fileData.data(), fileData.size());
		}

	}
	else {
		std::cout << "Save operation cancelled." << std::endl;
	}

	ImGui::CloseCurrentPopup();

}

void FileNode::ReplaceFile() {
	OPENFILENAME ofn;
	wchar_t szFile[260] = { 0 };
	LPWSTR pFile = szFile;

	mbstowcs_s(0, pFile, fileName.length() + 1, fileName.c_str(), _TRUNCATE);

	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lpstrFile = pFile;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.nMaxFile = 260;
	ofn.lpstrFilter = fileFilter;

	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		std::ifstream file(ofn.lpstrFile, std::ios::binary | std::ios::ate); // Open in binary mode!
		if (file.is_open()) {
			fileData.clear();
			std::streamsize size = file.tellg(); // Get file size
			fileData.resize(size);         // Resize vector to file size
			file.seekg(0, std::ios::beg);       // Go back to beginning
			file.read(fileData.data(), size); // Read data
			file.close();
			std::cout << "Replaced file! " << std::endl;
		}
		else {
			std::cout << "Error" << std::endl;
		}

	}

	ImGui::CloseCurrentPopup();

}

void FileNode::PopupOptions() {
	auto it = std::find(openFiles.begin(), openFiles.end(), this);
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::Button("Export", ImVec2(150, 20))) {
			ExportFile();
		}

		if (it != openFiles.end()) { // Ensure it exists before erasing
			if (ImGui::Button("Close", ImVec2(150, 20))) {
				closeNode(this);
			}
		}
		else {
			if (ImGui::Button("Replace", ImVec2(150, 20))) {
				ReplaceFile();
			}
		}



		//PopupOptionsEx();

		ImGui::EndPopup();
	}
}

void FileNode::Render() {
	std::string displayName = fileIcon + fileName;

	if (fileIsBigEndian) {
		displayName += " (X360/PS3)";
	}
	if (children.size() == 0) {

		ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
		if (ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_Leaf)) {
			PopupOptions();

			if (ImGui::IsItemClicked()) {
				selectedNode = this; // Update selected node
			}



			ImGui::TreePop();
		}
		ImGui::PopStyleColor();
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Text, TextColor);



		if (ImGui::TreeNode(displayName.c_str())) {

			PopupOptions();

			if (ImGui::IsItemClicked()) {
				selectedNode = this; // Update selected node
			}
			ImGui::PopStyleColor();
			for (FileNode* node : children) {
				node->Render();
			}

			ImGui::TreePop();
		}
		else {
			ImGui::PopStyleColor();
		}
	}

}

void FileNode::SetFileData(const std::vector<char>& data) {
	fileData = data;
}

const std::vector<char>& FileNode::GetFileData() const {
	return fileData;
}

UnkFileNode::UnkFileNode(std::string fName) : FileNode(fName) {

}

void UnkFileNode::LoadFile() {

}

void UnkFileNode::SaveFile() {

}

LY2FileNode::LY2FileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_ARRAY;
	TextColor = { 1.0f, 0.0f, 0.376f, 1.0f };
	nodeType = LY2;
	fileFilter = L"Layout Files (*.ly2)\0*.ly2;\0";
	ly2Flags = 0;
	propTypeCount = 0;
	mysteryPointer = 0;
	mysteryCount = 0;
}

void LY2FileNode::LoadFile() {
	BinaryReader reader(fileData, true);
	reader.SetEndianess(fileIsBigEndian);
	reader.Seek(0x4);
	ly2Flags = reader.ReadUINT32();
	propTypeCount = reader.ReadUINT32();
	mysteryPointer = reader.ReadUINT32();
	mysteryCount = reader.ReadUINT32();

	for (int x = 0; x < propTypeCount; x++) {
		LY2Node node = LY2Node();
		node.flag_a = reader.ReadUINT32();
		node.flag_b = reader.ReadUINT32();
		node.prop_category = reader.ReadString(2);
		node.prop_id = reader.ReadUINT16();
		node.offset = reader.ReadUINT32();
		node.count = reader.ReadUINT32();

		size_t pos = reader.Tell();
		reader.Seek(node.offset);
		for (int y = 0; y < node.count; y++) {
			LY2Instance instance = LY2Instance();
			instance.pos[0] = reader.ReadFloat();
			instance.pos[1] = reader.ReadFloat();
			instance.pos[2] = reader.ReadFloat();
			instance.scale[0] = reader.ReadFloat();
			instance.scale[1] = reader.ReadFloat();
			instance.scale[2] = reader.ReadFloat();
			instance.unknownB4C = reader.ReadUINT32();
			instance.unknownB4E = reader.ReadUINT32();
			instance.unknownB4F = reader.ReadUINT32();
			instance.unknownB4G = reader.ReadINT32();

			node.instances.push_back(instance);


		}
		nodes.push_back(node);
		reader.Seek(pos);

	}

	reader.Seek(mysteryPointer);
	for (int x = 0; x < mysteryCount; x++) {
		LY2ExData ex = LY2ExData();
		ex.a = reader.ReadUINT16();
		ex.b = reader.ReadUINT16();
		ex.c = reader.ReadUINT32();
		ex.d = reader.ReadUINT32();
		extradata.push_back(ex);
	}

}

void LY2FileNode::SaveFile() {
	if (!isEdited) {
		return;
	}
	BinaryWriter* writer = new BinaryWriter();

	writer->WriteString("LY2");
	writer->WriteByteZero();
	writer->WriteINT32(4);
	writer->WriteUINT32(static_cast<uint32_t>(nodes.size()));

	int extradataoffset = 20;
	int mainchunkend = 20;
	for (LY2Node& node : nodes) {
		extradataoffset += (20 + (40 * static_cast<int>(node.instances.size())));
		mainchunkend += 20;

	}
	writer->WriteUINT32(extradataoffset);
	writer->WriteUINT32(static_cast<uint32_t>(extradata.size()));
	int offset = 0;
	for (LY2Node& node : nodes) {
		writer->WriteUINT32(node.flag_a);
		writer->WriteUINT32(node.flag_b);
		writer->WriteString(node.prop_category);
		writer->WriteUINT16(node.prop_id);
		writer->WriteUINT32(mainchunkend + offset);
		writer->WriteUINT32(static_cast<uint32_t>(node.instances.size()));
		offset += 40 * static_cast<int>(node.instances.size());
	}
	for (LY2Node& node : nodes) {
		for (LY2Instance& inst : node.instances) {
			writer->WriteFloat(inst.pos[0]);
			writer->WriteFloat(inst.pos[1]);
			writer->WriteFloat(inst.pos[2]);
			writer->WriteFloat(inst.scale[0]);
			writer->WriteFloat(inst.scale[1]);
			writer->WriteFloat(inst.scale[2]);
			// TODO: Rotation encoding
			writer->WriteUINT32(inst.unknownB4C);
			writer->WriteUINT32(inst.unknownB4E);
			writer->WriteUINT32(inst.unknownB4F);
			writer->WriteINT32(inst.unknownB4G);
		}
	}
	for (LY2ExData& exData : extradata) {
		writer->WriteUINT16(exData.a);
		writer->WriteUINT16(exData.b);
		writer->WriteUINT32(exData.c);
		writer->WriteUINT32(exData.d);
	}

	fileData = writer->GetData();
}

void LY2FileNode::RenderGUI() {
	if (ImGui::BeginTabBar("ly2_bar")) {
		if (ImGui::BeginTabItem("Object Editor")) {
			int i = 0;
			for (LY2Node& node : nodes) {
				for (int j = 0; j < node.instances.size();) {
					LY2Instance& instance = node.instances[j];
					ImGui::PushID(i);

					if (ImGui::CollapsingHeader((node.prop_category + std::format("{:04x}", node.prop_id)).c_str())) {
						isEdited = true;
						ImGui::InputFloat3("Position", instance.pos, "%.2f");
						ImGui::InputFloat3("Scale", instance.scale, "%.2f");
						//ImGui::InputFloat3("Rotation", instance.rot, "%.2f");

						if (ImGui::Button("-")) {
							node.instances.erase(node.instances.begin() + j);
							ImGui::PopID();
							continue;
						}
					}

					ImGui::PopID();
					j++;
					i += 1;
				}


			}
			ImGui::PushID(i + 1);
			ImGui::SetNextItemWidth(60.0f);
			ImGui::InputText("", input_id, 7);
			ImGui::SameLine();
			if (ImGui::Button("+")) {
				isEdited = true;
				std::string hexStr(input_id + 2, 4);
				LY2Node node = LY2Node();
				node.prop_id = static_cast<unsigned short>(std::stoi(hexStr, nullptr, 16));
				node.prop_category = std::string(input_id, 2);
				node.instances.push_back(LY2Instance());
				nodes.push_back(node);


			}
			ImGui::PopID();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Group Editor")) {
			int i = 0;
			for (LY2Node& node : nodes) {
				if (node.instances.empty()) {
					continue; // Ignore
				}
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Group " + std::to_string(i) + " " + node.prop_category + std::format("{:04x}", node.prop_id)).c_str())) {
					ImGui::Text("Item Count %zu", node.instances.size());
					ImGui::InputInt("Flag A", &node.flag_a);
					ImGui::InputInt("Flag B", &node.flag_b);
				}
				ImGui::PopID();
				i += 1;
			}

			if (ImGui::Button("Optimize Groups")) {
				// Basically, every group with equal IDs and flags can be combined into one.
				for (size_t j = 0; j < nodes.size(); ++j) {
					if (nodes[j].instances.empty()) continue;

					for (size_t k = j + 1; k < nodes.size(); ++k) {
						if (nodes[k].flag_a == nodes[j].flag_a &&
							nodes[k].flag_b == nodes[j].flag_b &&
							nodes[k].prop_id == nodes[j].prop_id &&
							nodes[k].prop_category == nodes[j].prop_category) {

							nodes[j].instances.insert(nodes[j].instances.end(), nodes[k].instances.begin(), nodes[k].instances.end());

							nodes[k].instances.clear();
						}
					}
				}
			}
			ImGui::SameLine();
			HelpMarker("Combines groups with matching flags and IDs into one group to optimize the file");

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

}

UvdFileNode::UvdFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_LIBRARY;
	TextColor = { 1.0f, 0.0f, 0.376f, 1.0f };
	nodeType = UVD;
	fileFilter = L"UI Element File(*.uvd)\0*.uvd;\0";
}

void UvdFileNode::LoadFile() {
	BinaryReader reader(fileData, fileIsBigEndian);
	reader.Seek(0x4);
	uint32_t entriesCount = reader.ReadUINT32();
	uint32_t entriesOffset = reader.ReadUINT32();
	uint32_t texturesOffset = reader.ReadUINT32();
	uint32_t textureCount = (static_cast<uint32_t>(reader.GetSize()) - texturesOffset) / 36;

	reader.Seek(texturesOffset);
	for (uint32_t a = 0; a < textureCount; a++) {
		UvdTexture texture = UvdTexture();
		texture.name = reader.ReadString(32);
		texture.id = reader.ReadUINT32();
		uvdTextures.push_back(texture);
	}

	reader.Seek(entriesOffset);
	for (uint32_t a = 0; a < entriesCount; a++) {
		UvdEntry texture = UvdEntry();
		texture.Read(reader);
		uvdEntries.push_back(texture);
	}
}

void UvdFileNode::RenderGUI() {
	if (ImGui::BeginTabBar("uvd_editor_bar")) {
		if (ImGui::BeginTabItem("UVD Entries")) {
			for (UvdEntry entry : uvdEntries) {
				if (textureMap.find(entry.textureID) != textureMap.end()) {
					D3DSURFACE_DESC desc;
					textureMap[entry.textureID]->GetLevelDesc(0, &desc);  // Get info for mip level 0 (the base level)
					UINT texWidth = desc.Width;
					UINT texHeight = desc.Height;
					ImVec2 uv0(entry.x / (float)texWidth, entry.y / (float)texHeight);
					ImVec2 uv1((entry.x + entry.width) / (float)texWidth, (entry.y + entry.height) / (float)texHeight);
					ImGui::Image((ImTextureID)(intptr_t)textureMap[entry.textureID], ImVec2(64, 64), uv0, uv1);
				}
				else {
					ImGui::Image((ImTextureID)(intptr_t)textureMap[0], ImVec2(64, 64));
				}

				ImGui::SameLine();
				ImGui::Text("ID: %d", entry.ID);

				if (ImGui::TreeNode(entry.name.c_str())) {
					ImGui::TreePop();
				}

			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void UvdFileNode::SaveFile() {

}

UidFileNode::UidFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_EDITOR_LAYOUT;
	TextColor = { 1.0f, 0.0f, 0.376f, 1.0f };
	nodeType = UID;
	fileFilter = L"User Interface Description(*.uid)\0*.uid;\0";
}

void UidFileNode::LoadFile() {
	BinaryReader reader(fileData, fileIsBigEndian);

	uidHeader.Read(reader);

	if (uidHeader.offset1 > 0) {
		reader.Seek(uidHeader.offset1);
		for (int i = 0; i < uidHeader.size1; i++) {
			UIDEntry1 uid1;
			uid1.Read(reader);
			UIDEntry1List.push_back(uid1);
		}			
		int endOffset = uidHeader.offset2;
		if (endOffset == 0)
			endOffset = uidHeader.offset3;
		if (endOffset == 0)
			endOffset = static_cast<int>(reader.GetSize());

		std::vector<int> allOffsets;
		for (UIDEntry1& entry : UIDEntry1List) {
			if (entry.data1Offset != 0) {
				allOffsets.push_back(entry.data1Offset);
			}
			if (entry.data2Offset != 0) {
				allOffsets.push_back(entry.data2Offset);
			}
			if (entry.data3Offset != 0) {
				allOffsets.push_back(entry.data3Offset);
			}
		}
		allOffsets.push_back(endOffset);

		std::unordered_map<int, int> offsetSizes;
		for (size_t i = 0; i + 1 < allOffsets.size(); ++i) {
			int offset = allOffsets[i];
			int size = allOffsets[i + 1] - offset;
			offsetSizes[offset] = size;
		}

		for (UIDEntry1& entry : UIDEntry1List) {
			entry.readAdditionalData(reader, offsetSizes);
		}

	}
	if (uidHeader.offset2 > 0) {
		reader.Seek(uidHeader.offset2);
		for (int i = 0; i < uidHeader.size2; i++) {
			UIDEntry2 uid2;
			uid2.Read(reader);
			UIDEntry2List.push_back(uid2);
		}

	}
	if (uidHeader.offset3 > 0) {
		reader.Seek(uidHeader.offset3);
		for (int i = 0; i < uidHeader.size3; i++) {
			UIDEntry3 uid3;
			uid3.Read(reader);
			UIDEntry3List.push_back(uid3);
		}

	}
}

void UidFileNode::SaveFile() {
	if (!isEdited) {
		return;
	}

	BinaryWriter* writer = new BinaryWriter(fileIsBigEndian);
	UIDHeader header;
	header.size1 = static_cast<int>(UIDEntry1List.size());
	header.size2 = static_cast<int>(UIDEntry2List.size());
	header.size3 = static_cast<int>(UIDEntry3List.size());
	header.u0 = uidHeader.u0;
	header.u1 = uidHeader.u1;
	header.frameLength = uidHeader.frameLength;
	header.width = uidHeader.width;
	header.height = uidHeader.height;
	header.u2 = uidHeader.u2;

	header.offset1 = 36;		
	std::vector<int> dataPositions;
	int pointer = 36 + (432 * static_cast<int>(UIDEntry1List.size()));
	for (UIDEntry1& uid : UIDEntry1List) {
		if (uid.data1.data.size() > 0) {
			dataPositions.push_back(pointer);
			pointer += static_cast<int>(uid.data1.data.size());
		}
		else {
			dataPositions.push_back(0);
		}
		if (uid.data2.data.size() > 0) {
			dataPositions.push_back(pointer);
			pointer += static_cast<int>(uid.data2.data.size());
		}
		else {
			dataPositions.push_back(0);
		}
		if (uid.data3.data.size() > 0) {
			dataPositions.push_back(pointer);
			pointer += static_cast<int>(uid.data3.data.size());
		}
		else {
			dataPositions.push_back(0);
		}
	}
	header.offset2 = pointer;
	header.offset3 = header.offset2 + (sizeof(UIDEntry2) * static_cast<int>(UIDEntry2List.size()));

	header.Write(writer);
	int i = 0;
	writer->Seek(header.offset1);
	for (UIDEntry1& uid : UIDEntry1List) {
		uid.data1Offset = dataPositions[i];
		i += 1;
		uid.data2Offset = dataPositions[i];
		i += 1;
		uid.data3Offset = dataPositions[i];
		i += 1;

		uid.Write(writer);
	}

	for (UIDEntry1& uid : UIDEntry1List) {
		uid.writeAdditionalData(writer);
	}

	writer->Seek(header.offset2);
	for (UIDEntry2 uid : UIDEntry2List) {
		uid.Write(writer);
	}

	writer->Seek(header.offset3);
	for (UIDEntry3 uid : UIDEntry3List) {
		uid.Write(writer);
	}

	fileData = writer->GetData();
}

void UidFileNode::RenderGUI() {
	int i = 0;
	isEdited = true;
	if (parent) {
		for (FileNode* child : parent->children) {
			if (child->nodeType == UVD) {
				pairedUVD = (UvdFileNode*)child;
				break;
			}
		}
	}

	ImGui::Checkbox("Visaulize UID Positions", &UIDVisualize);
	ImGui::GetForegroundDrawList()->AddText(ImVec2(800.0f, 900.0f), IM_COL32(255.0f, 255.0f, 255.0f,255.0f), "UID Preview Canvas");
	ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(800.0f, 100.0f), ImVec2(1440.0f, 450.0f), IM_COL32(2.0f, 2.0f, 2.0f, 255.0f));
	for (UIDEntry1& entry : UIDEntry1List) {
		bool foundUVD = false;
		UvdEntry associatedUVDEntry;
		ImGui::PushID(i);
		if (entry.data1.dataType == Entry1Type::UIDrawImage && pairedUVD) {
			if (textureMap.find(entry.data1.dataStructure.img.textureID) != textureMap.end()) {
				for (UvdEntry uvdEntry : pairedUVD->uvdEntries) {
					if (uvdEntry.ID == entry.data1.dataStructure.img.uvdID) {
						associatedUVDEntry = uvdEntry;
						D3DSURFACE_DESC desc;
						textureMap[uvdEntry.textureID]->GetLevelDesc(0, &desc);  // Get info for mip level 0 (the base level)
						UINT texWidth = desc.Width;
						UINT texHeight = desc.Height;
						ImVec2 uv0(uvdEntry.x / (float)texWidth, uvdEntry.y / (float)texHeight);
						ImVec2 uv1((uvdEntry.x + uvdEntry.width) / (float)texWidth, (uvdEntry.y + uvdEntry.height) / (float)texHeight);
						ImGui::Image((ImTextureID)(intptr_t)textureMap[entry.data1.dataStructure.img.textureID], ImVec2(17, 17), uv0, uv1, ImVec4(entry.rgb.r, entry.rgb.g, entry.rgb.b, entry.rgb.a));
						ImGui::SameLine();
						foundUVD = true;
						break;
					}
				}
			}
		}
		if (!foundUVD) {
			ImGui::ColorEdit4("", entry.rgb, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
		}			
		if (ImGui::TreeNode(("UID Entry: " + std::to_string(i) + " (" + MGRUI::Entry1TypeFriendlyFormatted[entry.data1Flag] + ")").c_str())) {
			ImGui::PushItemWidth(240.0f);
			ImGui::InputFloat3("Position", entry.position);
			ImGui::InputFloat3("Rotation", entry.rotation);
			ImGui::InputFloat3("Scale", entry.scale);
			ImGui::ColorEdit4("Color", entry.rgb);
			if (entry.data1.dataType == Entry1Type::UIDrawImage) {
				entry.data1.dataStructure.img.Render();
			}

			if (ImGui::TreeNode("Extra Data")) {
				if (entry.data3.data.size() > 0) {
					if (ImGui::TreeNode("Animation Data")) {
						if (ImGui::Button("Delete Animation Data")) {
							entry.data3.data.clear();
						}
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();

			if (ImGui::Button("Duplicate")) {
				UIDEntry1List.push_back(entry);
			}
		}

		ImGui::PopID();			
		if (UIDVisualize) {
			float scalex = entry.position.x * (640.0f / 1280.0f) + 800.0f;
			float scaley = entry.position.y * (360.0f / 720.0f) + 100.0f;

			if (foundUVD) {
				D3DSURFACE_DESC desc;
				textureMap[associatedUVDEntry.textureID]->GetLevelDesc(0, &desc);  // Get info for mip level 0 (the base level)
				UINT texWidth = desc.Width;
				UINT texHeight = desc.Height;
				ImVec2 uv0(associatedUVDEntry.x / (float)texWidth, associatedUVDEntry.y / (float)texHeight);
				ImVec2 uv1((associatedUVDEntry.x + associatedUVDEntry.width) / (float)texWidth, (associatedUVDEntry.y + associatedUVDEntry.height) / (float)texHeight);
				ImVec2 screenPos = ImVec2(scalex, scaley); // top-left position
				ImVec2 screenEnd = ImVec2(screenPos.x + (associatedUVDEntry.width * entry.scale.x), screenPos.y + (associatedUVDEntry.height * entry.scale.y));
				foundUVD = true;
				ImGui::GetForegroundDrawList()->AddImage((ImTextureID)(intptr_t)textureMap[entry.data1.dataStructure.img.textureID], screenPos, screenEnd, uv0, uv1, IM_COL32_WHITE);
			}
			else {
				if (false) {
					ImGui::GetForegroundDrawList()->AddText(ImVec2(scalex, scaley), IM_COL32(entry.rgb.r * 255.0f, entry.rgb.g * 255.0f, entry.rgb.b * 255.0f, entry.rgb.a * 255.0f), ("UID Entry: " + std::to_string(i)).c_str());
				}					
			}
		}
		i += 1;
	}

}

MotFileNode::MotFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_RECORD;
	TextColor = { 1.0f, 0.0f, 0.376f, 1.0f };
	nodeType = MOT;
	fileFilter = L"XSI Motion Files(*.mot)\0*.mot;\0";
}

void MotFileNode::LoadFile() {

}

void MotFileNode::SaveFile() {

}

BxmFileNode::BxmFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_CODE;
	TextColor = { 1.0f, 0.0f, 0.914f, 1.0f };
	fileIsBigEndian = false;
	nodeType = BXM;
	fileFilter = L"Binary XML Files(*.bxm)\0*.bxm;\0";
}

std::string BxmFileNode::ConvertToXML(BXMInternal::XMLNode* node, int indentLevel) {
	if (!node) return "";

	std::string indent(indentLevel * 2, ' '); // Indentation for readability
	std::string xml = indent + "<" + node->name;

	// Process attributes
	for (const auto& attr : node->childAttributes) {
		if (!attr) continue;
		xml += " " + attr->name + "=\"" + attr->value + "\"";
	}

	if (node->childNodes.empty() && node->value.empty()) {
		xml += "/>\n"; // Self-closing tag
		return xml;
	}

	xml += ">";

	// Add value if present
	if (!node->value.empty()) {
		xml += node->value;

	}

	// Process child nodes recursively
	if (!node->childNodes.empty()) {
		xml += "\n";
		for (const auto& child : node->childNodes) {
			xml += ConvertToXML(child, indentLevel + 1);
		}
		xml += indent;
	}

	xml += "</" + node->name + ">\n";

	return xml;
}

BXMInternal::XMLNode* BxmFileNode::ReadXMLNode(BinaryReader reader, BXMInternal::XMLNode* parent) {
	BXMInternal::XMLNode* node = new BXMInternal::XMLNode();
	node->parent = parent;
	int childCount = reader.ReadUINT16();
	int firstChildIndex = reader.ReadUINT16();
	int attributeNumber = reader.ReadUINT16();
	int dataIndex = reader.ReadUINT16();

	reader.Seek(dataOffset + (dataIndex * 4)); // Seek to data position

	int nameOffset = reader.ReadUINT16();
	int valueOffset = reader.ReadUINT16();

	if (nameOffset != 0xFFFF) {
		reader.Seek(stringOffset + nameOffset);
		node->name = reader.ReadNullTerminatedString();
	}
	if (valueOffset != 0xFFFF) {
		reader.Seek(stringOffset + valueOffset);
		node->value = reader.ReadNullTerminatedString();
	}

	for (int i = 0; i < attributeNumber; i++) {
		reader.Seek(dataOffset + ((dataIndex + i + 1) * 4));
		nameOffset = reader.ReadUINT16();
		valueOffset = reader.ReadUINT16();
		BXMInternal::XMLAttribute* attrib = new BXMInternal::XMLAttribute();

		if (nameOffset != 0xFFFF) {
			reader.Seek(stringOffset + nameOffset);
			attrib->name = reader.ReadNullTerminatedString();
		}
		if (valueOffset != 0xFFFF) {
			reader.Seek(stringOffset + valueOffset);
			attrib->value = reader.ReadNullTerminatedString();
		}
		node->childAttributes.push_back(attrib);
	}
	for (int i = 0; i < childCount; i++) {
		reader.Seek(infoOffset + ((firstChildIndex + i) * 8));
		node->childNodes.push_back(ReadXMLNode(reader, node));

	}
	return node;
}

std::vector<BXMInternal::XMLNode*> BxmFileNode::FlattenXMLTree(BXMInternal::XMLNode* root) {
	std::vector<BXMInternal::XMLNode*> output;
	std::queue<BXMInternal::XMLNode*> q;
	q.push(root);

	while (!q.empty()) {
		BXMInternal::XMLNode* node = q.front();
		q.pop();
		output.push_back(node);

		for (BXMInternal::XMLNode* child : node->childNodes) {
			q.push(child);
		}
	}
	return output;
}

void BxmFileNode::RenderGUI() {
	if (baseNode->name == "SubPhaseInfoRoot") {
		isEdited = true;
		ImGui::Text("Phase Editor");
		BXMInternal::XMLNode* infoList = baseNode->childNodes[0];
		for (BXMInternal::XMLNode* subNode : infoList->childNodes) {
			if (ImGui::CollapsingHeader(subNode->childAttributes[0]->value.c_str())) {
				for (const std::string& paramName : BXMInternal::possibleParams) {
					// Step 2: Search for this parameter in the subNode
					BXMInternal::XMLNode* foundParam = nullptr;
					for (BXMInternal::XMLNode* existingParam : subNode->childNodes) {
						if (existingParam->name == paramName) {
							foundParam = existingParam;
							break;
						}
					}

					// If not found, create a dummy node (only for editing)
					if (!foundParam) {
						foundParam = new BXMInternal::XMLNode();
						foundParam->name = paramName;
						foundParam->value = ""; // default empty
						foundParam->parent = subNode;
						subNode->childNodes.push_back(foundParam);
					}

					ImGui::PushID(foundParam);

					std::string& paramValue = foundParam->value;						
					if (paramName == "RoomNo") {
						std::vector<std::string> rooms = BXMInternal::SplitString(paramValue, ' ');

						// Resize buffer to match rooms
						roomBuffers.resize(rooms.size());
						for (size_t i = 0; i < rooms.size(); ++i) {
							std::strncpy(roomBuffers[i].data(), rooms[i].c_str(), 32);
							roomBuffers[i][31] = '\0'; // Ensure null-termination
						}

						std::string newRoomData;

						for (size_t i = 0; i < roomBuffers.size(); ++i) {
							ImGui::InputText(("Room " + std::to_string(i)).c_str(), roomBuffers[i].data(), 32);
							ImGui::SameLine();
							if (ImGui::Button(("-##" + std::to_string(i)).c_str())) {
								roomBuffers.erase(roomBuffers.begin() + i);
								--i;
								continue;
							}
							newRoomData += std::string(roomBuffers[i].data()) + " ";
						}

						paramValue = newRoomData;

						if (ImGui::Button("New Room")) {
							std::array<char, 32> newRoom{};
							std::strncpy(newRoom.data(), "r000", 32);
							roomBuffers.push_back(newRoom);
						}
					}
					else if (paramName == "isRestartPoint") {
						bool checked = (paramValue == "YES");
						if (ImGui::Checkbox(paramName.c_str(), &checked)) {
							paramValue = checked ? "YES" : "NO";
						}
					}
					else if (paramName == "CameraEnable" || paramName == "CameraXEnable" || paramName == "CameraYEnable") {
						bool checked = (paramValue == "ON");
						if (ImGui::Checkbox(paramName.c_str(), &checked)) {
							paramValue = checked ? "ON" : "OFF";
						}
					}
					else if (paramName == "isPlWaitPayment") {
						bool checked = (paramValue == "YES");
						if (ImGui::Checkbox(paramName.c_str(), &checked)) {
							paramValue = checked ? "YES" : "NO";
						}
					}
					else if (paramName == "CameraYaw") {
						float yaw = paramValue.empty() ? 0.0f : std::stof(paramValue);
						if (ImGui::InputFloat(paramName.c_str(), &yaw)) {
							paramValue = std::to_string(yaw);
						}
					}
					else {
						ImGui::Text("%s: %s", paramName.c_str(), paramValue.c_str());
					}

					ImGui::PopID();
				}
			}
		}
	}
}

void BxmFileNode::LoadFile() {
	BinaryReader reader(fileData, true);
	reader.Seek(0x8);
	int nodeCount = reader.ReadUINT16();
	int dataCount = reader.ReadUINT16();
	reader.Skip(sizeof(int)); // TODO: dataSize

	infoOffset = 16;
	dataOffset = 16 + 8 * nodeCount;
	stringOffset = 16 + 8 * nodeCount + dataCount * 4;

	baseNode = ReadXMLNode(reader, nullptr);
	xmlData = ConvertToXML(baseNode);
	xmlBuffer.resize(xmlData.size() + 1); // +1 for null termination
	std::copy(xmlData.begin(), xmlData.end(), xmlBuffer.begin()); // Copy new data into buffer
	xmlBuffer[xmlBuffer.size() - 1] = '\0';

	return;
}

void BxmFileNode::SaveFile() {
	if (!isEdited) {
		return;
	}
	BinaryWriter* writer = new BinaryWriter(true);
	writer->WriteString("BXM");
	writer->WriteByteZero();
	writer->WriteUINT32(0);
	std::vector<BXMInternal::XMLNode*> nodes = FlattenXMLTree(baseNode);
	std::unordered_set<std::string> uniqueStrings;
	for (BXMInternal::XMLNode* node : nodes) {
		uniqueStrings.insert(node->name);
		uniqueStrings.insert(node->value);
		for (BXMInternal::XMLAttribute* attrib : node->childAttributes) {
			uniqueStrings.insert(attrib->name);
			uniqueStrings.insert(attrib->value);
		}
	}

	std::unordered_map<std::string, int> stringOffsets;
	int offsetTicker = 0;
	for (std::string nodeString : uniqueStrings) {
		if (nodeString.empty()) {
			stringOffsets[nodeString] = 0xFFFF;
			offsetTicker += 1; // I have no idea why this happens but it works 
		}
		else {
			stringOffsets[nodeString] = offsetTicker;
			offsetTicker += (nodeString.size() + 1);
		}
	}

	struct DataInfo {
		int name;
		int value;
	};

	std::vector<DataInfo> dataInfos;
	std::unordered_map <BXMInternal::XMLNode*, int> nodeInfoToDataIndice;
	int i = 0;
	for (BXMInternal::XMLNode* node : nodes) {
		dataInfos.push_back({ stringOffsets[node->name], stringOffsets[node->value] });
		nodeInfoToDataIndice[node] = i;
		for (BXMInternal::XMLAttribute* attrib : node->childAttributes) {
			dataInfos.push_back({ stringOffsets[attrib->name], stringOffsets[attrib->value] });
			i++;
		}			
		i++;
	}

	writer->WriteUINT16(nodes.size());
	writer->WriteUINT16(dataInfos.size());
	writer->WriteUINT32(offsetTicker);

	struct BXMNodeInfo {
		int childSize;
		int firstChildIdx;
		int attributeSize;
		int dataIdx;
	};

	std::unordered_map<BXMInternal::XMLNode*, BXMNodeInfo> nodeInfos;
	std::unordered_map<BXMInternal::XMLNode*, int> nodeToIndex;
	for (int i = 0; i < nodes.size(); ++i) {
		nodeToIndex[nodes[i]] = i;
		nodeInfos[nodes[i]] = { (int)nodes[i]->childNodes.size(), -1, (int)nodes[i]->childAttributes.size(), nodeInfoToDataIndice[nodes[i]] };
	}

	for (BXMInternal::XMLNode* node : nodes) {
		BXMNodeInfo& info = nodeInfos[node];
		int nextIndex = -1;

		if (!node->childNodes.empty()) {
			BXMInternal::XMLNode* firstChild = node->childNodes.front();
			nextIndex = nodeToIndex[firstChild];
		}
		else {
			BXMInternal::XMLNode* parent = node->parent;

			if (parent != nullptr) {
				BXMInternal::XMLNode* lastChild = parent->childNodes.back();
				int lastChildIndex = nodeToIndex[lastChild];
				nextIndex = lastChildIndex + 1;
			}
			else {
				nextIndex = static_cast<int>(nodes.size());
			}
		}

		info.firstChildIdx = nextIndex;
	}

	for (BXMInternal::XMLNode* node : nodes) {
		writer->WriteUINT16(nodeInfos[node].childSize);
		writer->WriteUINT16(nodeInfos[node].firstChildIdx);
		writer->WriteUINT16(nodeInfos[node].attributeSize);
		writer->WriteUINT16(nodeInfos[node].dataIdx);
	}

	for (DataInfo info : dataInfos) {
		writer->WriteUINT16(info.name);
		writer->WriteUINT16(info.value);
	}

	for (std::string str : uniqueStrings) {
		writer->WriteString(str);
		writer->WriteByteZero();
	}

	fileData = writer->GetData();
}

WtbFileNode::WtbFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_FILE_MEDIA;
	nodeType = WTB;
	TextColor = { 1.0f, 0.0f, 0.7f, 1.0f };
	fileFilter = L"Texture Container(*.wtb)\0*.wtb;\0";
}

void WtbFileNode::LoadFile() {
	BinaryReader reader(fileData, false);
	reader.SetEndianess(fileIsBigEndian);

	reader.Seek(0x4);
	if (reader.ReadUINT32() == 1) {
		textureCount = reader.ReadUINT32();
		int offsetTextureOffsets = reader.ReadUINT32();
		int offsetTextureSizes = reader.ReadUINT32();
		int offsetTextureFlags = reader.ReadUINT32();
		int offsetTextureIdx = reader.ReadUINT32();
		reader.Skip(sizeof(int)); // TODO: offsetTextureInfo

		reader.Seek(offsetTextureOffsets);
		for (int x = 0; x < textureCount; x++) {
			textureOffsets.push_back(reader.ReadUINT32());
		}

		reader.Seek(offsetTextureSizes);
		for (int x = 0; x < textureCount; x++) {
			textureSizes.push_back(reader.ReadUINT32());
		}

		reader.Seek(offsetTextureFlags);
		for (int x = 0; x < textureCount; x++) {
			textureFlags.push_back(reader.ReadUINT32());
		}

		reader.Seek(offsetTextureIdx);
		for (int x = 0; x < textureCount; x++) {
			textureIdx.push_back(reader.ReadUINT32());
		}

		for (int x = 0; x < textureCount; x++) {
			reader.Seek(textureOffsets[x]);
			textureData.push_back(reader.ReadBytes(textureSizes[x]));
		}
	}
}

void WtbFileNode::SaveFile() {

}

WemFileNode::WemFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_MUSIC;
	nodeType = WEM;
	fileFilter = L"WWISE Audio Container(*.wem)\0*.wem;\0";
}

void WemFileNode::LoadFile() {

}

void WemFileNode::SaveFile() {

}

WmbFileNode::WmbFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_SYMBOL_METHOD;
	TextColor = { 1.0f, 0.671f, 0.0f, 1.0f };
	nodeType = WMB;
	fileFilter = L"Platinum Model File(*.wmb)\0*.wmb;\0";
}

void WmbFileNode::LoadFile() {
	BinaryReader reader(fileData, true);
	reader.SetEndianess(fileIsBigEndian);

	WMBHeader header = WMBHeader();
	header.Read(reader);

	reader.Seek(header.offsetVertexGroups);
	std::vector<WMBVertexGroup> vertexGroups;
	for (uint32_t i = 0; i < header.numVertexGroups; i++) {
		WMBVertexGroup vtxGroup;
		vtxGroup.Read(reader);
		vertexGroups.push_back(vtxGroup);
	}

	reader.Seek(header.offsetBatches);
	std::vector<WMBBatch> batches;
	for (uint32_t i = 0; i < header.numBatches; i++) {
		WMBBatch itm;
		itm.Read(reader);
		batches.push_back(itm);
	}

	reader.Seek(header.offsetBatchDescription);
	unsigned int offsetBatchData = reader.ReadUINT32();

	reader.Seek(offsetBatchData);
	std::vector<WMBBatchData> batchDatas;
	for (uint32_t i = 0; i < header.numBatches; i++) {
		WMBBatchData itm;
		itm.Read(reader);
		batchDatas.push_back(itm);
	}

	reader.Seek(header.offsetMeshes);
	std::vector<WMBMesh> meshes;
	for (uint32_t i = 0; i < header.numMeshes; i++) {
		WMBMesh itm;
		itm.Read(reader);
		meshes.push_back(itm);
	}

	reader.Seek(header.offsetTextures);
	std::vector<WMBTexture> textures;
	for (uint32_t i = 0; i < header.numTextures; i++) {
		WMBTexture itm;
		itm.Read(reader);
		textures.push_back(itm);
	}


	reader.Seek(header.offsetMaterials);
	for (uint32_t i = 0; i < header.numMaterials; i++) {
		WMBMaterial mat;
		mat.Read(reader);
		size_t place = reader.Tell();
		CTDMaterial cmat = CTDMaterial();

		reader.Seek(mat.offsetShaderName);
		cmat.shader_name = reader.ReadString(16);
		reader.Seek(mat.offsetTextures);
		std::vector<WMBTexture> textureMappings;
		for (int j = 0; j < mat.numTextures; j++) {
			WMBTexture itm;
			itm.Read(reader);
			textureMappings.push_back(itm);
		}

		for (WMBTexture& tex : textureMappings) {
			cmat.texture_data[tex.flag] = textures[tex.id].id;
		}
		materials.push_back(cmat);
		reader.Seek(place);
	}
	for (WMBMesh& mesh : meshes) {
		CruelerMesh* ctdmesh = new CruelerMesh();
		ctdmesh->vtxfmt = header.vertexFormat;
		reader.Seek(mesh.offsetName);
		ctdmesh->name = reader.ReadNullTerminatedString();
		// Materials
		reader.Seek(mesh.offsetMaterials);
		for (uint32_t x = 0; x < mesh.numMaterials; x++) {
			ctdmesh->materials.push_back(&materials[reader.ReadUINT16()]);
		}
		std::vector<unsigned short> batchIDs;

		reader.Seek(mesh.offsetBatches); // this format sucks :fire:
		batchIDs = reader.ReadUINT16Array(mesh.numBatches);

		for (unsigned short meshBatchID : batchIDs) {
			CruelerBatch* ctdbatch = new CruelerBatch();

			WMBBatch& activeBatch = batches[meshBatchID];
			WMBBatchData& activeBatchData = batchDatas[meshBatchID];
			WMBVertexGroup& activeVtxGroup = vertexGroups[activeBatch.vertexGroupIndex];

			ctdbatch->vertexCount = activeVtxGroup.numVertexes;
			ctdbatch->indexCount = activeVtxGroup.numIndexes;				
			ctdbatch->materialID = activeBatchData.materialIndex;

			reader.Seek(activeVtxGroup.offsetIndexes + (sizeof(short) * activeBatch.indexStart));
			std::vector<unsigned short> indices;
			for (uint32_t x = 0; x < activeBatch.numIndices; x++) {
				indices.push_back(reader.ReadUINT16());
			}

			if (fileIsBigEndian) { // what the actual fuck platinum
				std::vector<unsigned short> chain;
				std::vector<unsigned short> newIndices;
				bool reverse = false;
				for (unsigned short& indice : indices) {
					if (indice == 0xFFFF) {
						chain.clear();
						reverse = false;
						continue;
					}
					chain.push_back(indice);
					if (chain.size() > 3) {
						chain.erase(chain.begin());
					}
					if (chain.size() == 3) {
						if (reverse) {
							std::reverse(chain.begin(), chain.end());
						}
						newIndices.insert(newIndices.end(), chain.begin(), chain.end());
						if (reverse) {
							std::reverse(chain.begin(), chain.end());
						}
						reverse = !reverse;

					}
				}

				indices = newIndices;
			}
			ctdbatch->indexCount = static_cast<int>(indices.size());
			ctdbatch->indexes = indices;

			g_pd3dDevice->CreateIndexBuffer(static_cast<UINT>(indices.size()) * sizeof(short), 0,
				D3DFMT_INDEX16, D3DPOOL_MANAGED,
				&ctdbatch->indexBuffer, nullptr);
			void* pIndexData = nullptr;
			ctdbatch->indexBuffer->Lock(0, 0, &pIndexData, 0);

			std::memcpy(pIndexData, indices.data(), indices.size() * sizeof(short));
			ctdbatch->indexBuffer->Unlock();

			// vertex format lore
			if (header.vertexFormat == 263) {
				reader.Seek(activeVtxGroup.offsetVertexes + activeBatch.vertexStart * sizeof(WMBVertexA));
				ctdmesh->structSize = sizeof(CUSTOMVERTEX);

				std::vector<WMBVertexA> vertexes;
				for (uint32_t i = 0; i < activeBatch.numVertices; i++) {
					WMBVertexA vtx;
					vtx.Read(reader);
					vertexes.push_back(vtx);
				}


				std::vector<CUSTOMVERTEX> convertedVtx;
				for (WMBVertexA& vertex : vertexes) {
					CUSTOMVERTEX cvtx;
					cvtx.x = vertex.position.x;
					cvtx.y = vertex.position.y;
					cvtx.z = vertex.position.z;
					cvtx.u = HelperFunction::HalfToFloat(vertex.uv.u);
					cvtx.v = HelperFunction::HalfToFloat(vertex.uv.v);
					cvtx.color = D3DCOLOR_RGBA(255, 255, 255, 255);
					convertedVtx.push_back(cvtx);
				}

				g_pd3dDevice->CreateVertexBuffer(activeBatch.numVertices * sizeof(CUSTOMVERTEX),
					0, 0,
					D3DPOOL_DEFAULT, &ctdbatch->vertexBuffer, NULL);

				void* pVertexData = nullptr;
				const HRESULT hr = ctdbatch->vertexBuffer->Lock(0, 0, &pVertexData, 0);
				if (S_OK != hr) {
					// TODO: Handle this
				}
				std::memcpy(pVertexData, convertedVtx.data(), activeBatch.numVertices * sizeof(CUSTOMVERTEX));
				ctdbatch->vertexBuffer->Unlock();
				ctdbatch->vertexes = convertedVtx;
				g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

			}
			else if (header.vertexFormat == 65847 || header.vertexFormat == 66359 || header.vertexFormat == 311) {

				reader.Seek(activeVtxGroup.offsetVertexes + activeBatch.vertexStart * sizeof(WMBVertex65847));

				ctdmesh->structSize = sizeof(CUSTOMVERTEX);

				std::vector<WMBVertex65847> vertexes;
				for (uint32_t i = 0; i < activeBatch.numVertices; i++) {
					WMBVertex65847 vtx;
					vtx.Read(reader);
					vertexes.push_back(vtx);
				}

				std::vector<CUSTOMVERTEX> convertedVtx;
				for (WMBVertex65847& vertex : vertexes) {
					CUSTOMVERTEX cvtx;
					cvtx.x = vertex.position.x;
					cvtx.y = vertex.position.y;
					cvtx.z = vertex.position.z;
					cvtx.u = HelperFunction::HalfToFloat(vertex.uv.u);
					cvtx.v = HelperFunction::HalfToFloat(vertex.uv.v);
					cvtx.color = D3DCOLOR_RGBA(255, 255, 255, 255);
					convertedVtx.push_back(cvtx);
				}

				g_pd3dDevice->CreateVertexBuffer(activeBatch.numVertices * sizeof(CUSTOMVERTEX),
					0, 0,
					D3DPOOL_DEFAULT, &ctdbatch->vertexBuffer, NULL);

				void* pVertexData = nullptr;
				const HRESULT hr = ctdbatch->vertexBuffer->Lock(0, 0, &pVertexData, 0);
				if (S_OK != hr) {
					// TODO: Handle this
				}
				std::memcpy(pVertexData, convertedVtx.data(), activeBatch.numVertices * sizeof(CUSTOMVERTEX));
				ctdbatch->vertexBuffer->Unlock();
				ctdbatch->vertexes = convertedVtx;
				g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
			}
			else if (header.vertexFormat == 66311) {

				reader.Seek(activeVtxGroup.offsetVertexes + activeBatch.vertexStart * sizeof(WMBVertex66311));
				ctdmesh->structSize = sizeof(CUSTOMVERTEX);

				std::vector<WMBVertex66311> vertexes;
				for (uint32_t i = 0; i < activeBatch.numVertices; i++) {
					WMBVertex66311 vtx;
					vtx.Read(reader);
					vertexes.push_back(vtx);
				}

				std::vector<CUSTOMVERTEX> convertedVtx;
				for (WMBVertex66311& vertex : vertexes) {
					CUSTOMVERTEX cvtx;
					cvtx.x = vertex.position.x;
					cvtx.y = vertex.position.y;
					cvtx.z = vertex.position.z;
					cvtx.u = HelperFunction::HalfToFloat(vertex.uv.u);
					cvtx.v = HelperFunction::HalfToFloat(vertex.uv.v);
					cvtx.color = D3DCOLOR_RGBA(255, 255, 255, 255);
					convertedVtx.push_back(cvtx);
				}

				g_pd3dDevice->CreateVertexBuffer(activeBatch.numVertices * sizeof(CUSTOMVERTEX),
					0, 0,
					D3DPOOL_DEFAULT, &ctdbatch->vertexBuffer, NULL);

				void* pVertexData = nullptr;
				const HRESULT hr = ctdbatch->vertexBuffer->Lock(0, 0, &pVertexData, 0);
				if (S_OK != hr) {
					// TODO: Handle this
				}
				std::memcpy(pVertexData, convertedVtx.data(), activeBatch.numVertices * sizeof(CUSTOMVERTEX));
				ctdbatch->vertexBuffer->Unlock();
				ctdbatch->vertexes = convertedVtx;
				g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
			}
			else if (header.vertexFormat == 65799) {
				// How platinum games felt after making 2 of the EXACT SAME DAMN VERTEX GROUPS EXCEPT ONE HAS AN EXTRA UV MAP 
				reader.Seek(activeVtxGroup.offsetVertexes + activeBatch.vertexStart * sizeof(WMBVertex65799));
				ctdmesh->structSize = sizeof(CUSTOMVERTEX);

				std::vector<WMBVertex65799> vertexes;
				for (uint32_t i = 0; i < activeBatch.numVertices; i++) {
					WMBVertex65799 vtx;
					vtx.Read(reader);
					vertexes.push_back(vtx);
				}

				std::vector<CUSTOMVERTEX> convertedVtx;
				for (WMBVertex65799& vertex : vertexes) {
					CUSTOMVERTEX cvtx;

					cvtx.x = vertex.position.x;
					cvtx.y = vertex.position.y;
					cvtx.z = vertex.position.z;
					cvtx.u = HelperFunction::HalfToFloat(vertex.uv.u);
					cvtx.v = HelperFunction::HalfToFloat(vertex.uv.v);
					cvtx.color = D3DCOLOR_RGBA(255, 255, 255, 255);
					convertedVtx.push_back(cvtx);
				}

				g_pd3dDevice->CreateVertexBuffer(activeBatch.numVertices * sizeof(CUSTOMVERTEX),
					0, 0,
					D3DPOOL_DEFAULT, &ctdbatch->vertexBuffer, NULL);

				void* pVertexData = nullptr;
				const HRESULT hr = ctdbatch->vertexBuffer->Lock(0, 0, &pVertexData, 0);
				if (S_OK != hr) {
					// TODO: Handle this
				}
				std::memcpy(pVertexData, convertedVtx.data(), activeBatch.numVertices * sizeof(CUSTOMVERTEX));
				ctdbatch->vertexBuffer->Unlock();
				ctdbatch->vertexes = convertedVtx;
				g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
			}
			ctdmesh->batches.push_back(ctdbatch);
		}
		displayMeshes.push_back(ctdmesh);
	}
}

void WmbFileNode::SaveFile() {

}

void WmbFileNode::PopupOptions() {
	auto it = std::find(openFiles.begin(), openFiles.end(), this);
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::Button("Export (WMB)", ImVec2(150, 20))) {
			ExportFile();
		}

		if (ImGui::Button("Export (FBX)", ImVec2(150, 20))) {
			FbxManager* manager = FbxManager::Create();
			FbxScene* scene = FbxScene::Create(manager, "WMB");
			scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
			FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
			manager->SetIOSettings(ios);
			ios->SetBoolProp(EXP_FBX_EMBEDDED, true);

			for (CruelerMesh* cmesh : displayMeshes) {
				for (CruelerBatch* batch : cmesh->batches) {
					// TODO: GLTF
					FbxSurfaceLambert* material = FbxSurfaceLambert::Create(scene, (cmesh->name + "_material").c_str());
					HelperFunction::WriteVectorToFile(rawTextureInfo[materials[batch->materialID].texture_data[0]], ("temp\\" + std::to_string(materials[batch->materialID].texture_data[0])));
					HelperFunction::WriteVectorToFile(rawTextureInfo[materials[batch->materialID].texture_data[2]], ("temp\\" + std::to_string(materials[batch->materialID].texture_data[2])));
					FbxFileTexture* abTexture = FbxFileTexture::Create(scene, "Albdeo");
					abTexture->SetFileName(("temp\\" + std::to_string(materials[batch->materialID].texture_data[0])).c_str());
					abTexture->SetTextureUse(FbxTexture::eStandard);
					abTexture->SetMappingType(FbxTexture::eUV);
					abTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
					abTexture->SetAlphaSource(FbxFileTexture::eNone);
					abTexture->SetSwapUV(false);
					material->Diffuse.ConnectSrcObject(abTexture);
					material->DiffuseFactor.Set(1.0);
					FbxFileTexture* nmTexture = FbxFileTexture::Create(scene, "Normal");
					nmTexture->SetFileName(("temp\\" + std::to_string(materials[batch->materialID].texture_data[2])).c_str());
					nmTexture->SetTextureUse(FbxTexture::eBumpNormalMap);
					nmTexture->SetMappingType(FbxTexture::eUV);
					nmTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
					nmTexture->SetAlphaSource(FbxFileTexture::eNone);
					nmTexture->SetSwapUV(false);
					material->BumpFactor.Set(1.0);

					material->NormalMap.ConnectSrcObject(nmTexture);

					material->TransparencyFactor.Set(0.0);
					material->TransparentColor.Set(FbxDouble3(0.0, 0.0, 0.0));

					FbxMesh* mesh = FbxMesh::Create(scene, (cmesh->name + "_mesh").c_str());						
					FbxGeometryElementUV* lUVDiffuseElement = mesh->CreateElementUV("Float2");						
					FBX_ASSERT(lUVDiffuseElement != NULL);
					lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
					lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);

					mesh->InitControlPoints(static_cast<int>(batch->vertexes.size()));
					for (int i = 0; i < batch->vertexes.size(); i++) {
						mesh->SetControlPointAt(FbxVector4(batch->vertexes[i].x, batch->vertexes[i].y, batch->vertexes[i].z), i);
						lUVDiffuseElement->GetDirectArray().Add(FbxVector2(batch->vertexes[i].u, 1.0 - batch->vertexes[i].v));
					}

					lUVDiffuseElement->GetIndexArray().SetCount(static_cast<int>(batch->indexes.size()));

					for (int i = 0; i < batch->indexes.size(); i+=3) {
						mesh->BeginPolygon();
						int idx0 = batch->indexes[i];
						int idx1 = batch->indexes[i + 1];
						int idx2 = batch->indexes[i + 2];

						mesh->AddPolygon(idx0);
						mesh->AddPolygon(idx1);
						mesh->AddPolygon(idx2);

						mesh->EndPolygon();
					}
					FbxNode* meshNode = FbxNode::Create(scene, cmesh->name.c_str());
					meshNode->AddMaterial(material);
					meshNode->SetNodeAttribute(mesh);
					scene->GetRootNode()->AddChild(meshNode);
				}
			}

			FbxExporter* exporter = FbxExporter::Create(manager, "");
			exporter->Initialize("output.fbx", -1, manager->GetIOSettings());
			exporter->Export(scene);
			exporter->Destroy();
		}

		if (it != openFiles.end()) { // Ensure it exists before erasing
			if (ImGui::Button("Close", ImVec2(150, 20))) {
				closeNode(this);
			}
		}
		else {
			if (ImGui::Button("Replace", ImVec2(150, 20))) {
				ReplaceFile();
			}
		}

		//PopupOptionsEx();

		ImGui::EndPopup();
	}
}

void WmbFileNode::RenderMesh() {
	rotationAngle += 0.01f;
	for (CruelerMesh* mesh : displayMeshes) {
		if (mesh->visibility) {
			for (CruelerBatch* batch : mesh->batches) {
				if (isSCR) {
					if (textureMap.find(mesh->materials[batch->materialID]->texture_data[0]) != textureMap.end()) {
						g_pd3dDevice->SetTexture(0, textureMap[mesh->materials[batch->materialID]->texture_data[0]]);
					}
					else {
						g_pd3dDevice->SetTexture(0, textureMap[0]);
					}
				}
				else {
					if (textureMap.find(materials[batch->materialID].texture_data[0]) != textureMap.end()) { // hack 
						g_pd3dDevice->SetTexture(0, textureMap[materials[batch->materialID].texture_data[0]]);
					}
					else {
						g_pd3dDevice->SetTexture(0, textureMap[0]);
					}
				}

				if (isSCR) {
					D3DXMATRIX matScale, matRot, matTrans, matWorld;
					D3DXMatrixScaling(&matScale, scaleOffset.x, scaleOffset.y, scaleOffset.z);
					D3DXMatrixRotationYawPitchRoll(&matRot, 0.0f, 0.0f, 0.0f);
					D3DXMatrixTranslation(&matTrans, meshOffset.x, meshOffset.y, meshOffset.z);
					matWorld = matScale * matRot * matTrans;
					g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
				}

				g_pd3dDevice->SetIndices(batch->indexBuffer);
				g_pd3dDevice->SetStreamSource(0, batch->vertexBuffer, 0, mesh->structSize);

				g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, batch->vertexCount, 0, batch->indexCount / 3);
			}
		}
	}
}

void WmbFileNode::RenderGUI() {
	if (isSCR) {
		ImGui::Text("WMB Meshes - %s", fileName.c_str());
	}
	else {
		ImGui::Text("WMB Meshes");
	}

	int i = 0;
	for (CruelerMesh* mesh : displayMeshes) {
		i += 1;
		ImGui::Text("%s. ", (std::to_string(i)).c_str());
		ImGui::SameLine();
		ImGui::Checkbox((mesh->name).c_str(), &mesh->visibility);
	}
}

void SCRMesh::Read(BinaryReader& br) {
	offset = br.ReadUINT32();
	name = br.ReadString(64);
	position.x = br.ReadFloat();
	position.y = br.ReadFloat();
	position.z = br.ReadFloat();
	rotation.x = br.ReadFloat();
	rotation.y = br.ReadFloat();
	rotation.z = br.ReadFloat();
	scale.x = br.ReadFloat();
	scale.y = br.ReadFloat();
	scale.z = br.ReadFloat();
}

ScrFileNode::ScrFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_LAYERS;
	TextColor = { 1.0f, 0.0f, 1.0f, 1.0f };
	nodeType = MOT;
	fileFilter = L"Platinum Stage File(*.scr)\0*.scr;\0";
}

void ScrFileNode::LoadFile() {
	BinaryReader reader(fileData, true);
	reader.SetEndianess(fileIsBigEndian);

	reader.Seek(0x6);

	int modelCount = reader.ReadUINT16();
	int offsetModelDef = reader.ReadUINT32();

	reader.Seek(offsetModelDef);
	std::vector<unsigned int> meshOffsets;

	for (int i = 0; i < modelCount; i++) {
		meshOffsets.push_back(reader.ReadUINT32());
	}

	std::vector<SCRMesh> meshes;
	for (int offset : meshOffsets) {
		reader.Seek(offset);
		SCRMesh mesh;
		mesh.Read(reader);
		meshes.push_back(mesh);
	}
	for (int i = 0; i < modelCount; i++) {
		reader.Seek(meshes[i].offset);
		int size = 0;
		if (i != meshes.size() - 1) {
			size = meshes[i + 1].offset - meshes[i].offset;
		}
		else {
			size = static_cast<int>(fileData.size()) - meshes[i].offset;
		}

		WmbFileNode* node = new WmbFileNode(std::string(meshes[i].name));
		node->isSCR = true;
		node->scrNode = this;
		node->meshOffset = meshes[i].position;
		node->scaleOffset = meshes[i].scale;
		node->meshRotation = meshes[i].rotation;
		node->fileIsBigEndian = fileIsBigEndian;
		node->SetFileData(reader.ReadBytes(size));
		node->LoadFile();
		children.push_back(node);
	}
}

void ScrFileNode::SaveFile() {

}

BnkFileNode::BnkFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_PACKAGE;
	TextColor = { 0.0f, 0.529f, 0.724f, 1.0f };
	nodeType = BNK;
	fileFilter = L"WWISE Audio Bank v72(*.bnk)\0*.bnk;\0";
}

void BnkFileNode::LoadFile() {
	BinaryReader reader(fileData, fileIsBigEndian);
	reader.Seek(0x4);
	int headerLength = reader.ReadUINT32();
	int wwiseVersion = reader.ReadUINT32();

	if (wwiseVersion != 72) {
		if (wwiseVersion == 1207959552) {
			reader.SetEndianess(true);
			reader.Seek(0x4);
			headerLength = reader.ReadUINT32();
			wwiseVersion = reader.ReadUINT32();
		}
		else {
			loadFailed = true;
			CTDLog::Log::getInstance().LogError("BNK failed version check! (Value at 0x10 isn't 72 but instead is " + std::to_string(wwiseVersion) + ")");
			return;
		}
	}
	reader.Seek(0x8 + headerLength);
	while (!reader.EndOfBuffer()) {
		std::string chunkType = reader.ReadString(4);
		size_t chunkSize = reader.ReadUINT32();
		size_t nextChunkPosition = reader.Tell() + chunkSize;

		if (chunkType == "DIDX") {
			std::vector<int> wemIDs;
			std::vector<int> wemOffsets;
			std::vector<int> wemSizes;
			if (chunkSize > 0) {
				int wemCount = static_cast<int>(chunkSize / 12);
				for (int i = 0; i < wemCount; i++) {
					wemIDs.push_back(reader.ReadUINT32());
					wemOffsets.push_back(reader.ReadUINT32());
					wemSizes.push_back(reader.ReadUINT32());
				}
				reader.ReadUINT32();
				reader.ReadUINT32();
				size_t baseOffset = reader.Tell();
				// The DATA chunk *should* always be after DIDX, if it isn't uhhh... kick sand?

				for (int i = 0; i < wemCount; i++) {
					reader.Seek(wemOffsets[i] + baseOffset);
					std::string wemName = std::to_string(wemIDs[i]) + ".wem";
					FileNode* childNode = HelperFunction::LoadNode(wemName, reader.ReadBytes(wemSizes[i]), false, false);
					if (childNode) {
						children.push_back(childNode);
					}

				}
			}
		}
		if (chunkType == "HIRC") {
			// Chunk lore
			int childrenCount = reader.ReadUINT32();
			for (int i = 0; i < childrenCount; i++) {
				int flag = reader.ReadINT8();
				int size = reader.ReadUINT32();
				size_t nextHircChunkPosition = reader.Tell() + size;
				reader.Skip(sizeof(int)); // TODO: uid

				if (flag == 0x4) {						
				}


				reader.Seek(nextHircChunkPosition);
			}
		}
		reader.Seek(nextChunkPosition);
	}

	/*std::string tmpHeader = "";
	int tmpLength = headerLength;
	reader.Seek(0x8);
	while (tmpHeader != "DIDX") {
		int offset = reader.Tell();
		reader.Seek(offset + headerLength);
		if (offset + headerLength + 4 >= fileData.size()) {
			break;
		}
		tmpHeader = reader.ReadString(4);
		tmpLength = reader.ReadUINT32();
	}

	// Checking the final header is actually DIDX
	if (tmpHeader == "DIDX") {
		if (tmpLength > 0) {
			int wemCount = (tmpLength / 12);
			for (int i = 0; i < wemCount; i++) {
				wemIDs.push_back(reader.ReadUINT32());
				wemOffsets.push_back(reader.ReadUINT32());
				wemSizes.push_back(reader.ReadUINT32());
			}
			reader.ReadUINT32();
			reader.ReadUINT32();
			int baseOffset = reader.Tell();

			for (int i = 0; i < wemCount; i++) {
				reader.Seek(wemOffsets[i] + baseOffset);
				std::string wemName = std::to_string(wemIDs[i]) + ".wem";
				FileNode* childNode = HelperFunction::LoadNode(wemName, reader.ReadBytes(wemSizes[i]), false, false);
				if (childNode) {
					children.push_back(childNode);
				}
			}
		}
	}*/
}

void BnkFileNode::SaveFile() {

}

DatFileNode::DatFileNode(std::string fName) : FileNode(fName) {
	fileIcon = ICON_CI_FOLDER;
	TextColor = { 0.0f, 0.98f, 0.467f, 1.0f };
	nodeType = DAT;
	fileFilter = L"Platinum File Container(*.dat, *.dtt, *.eff, *.evn, *.eft)\0*.dat;*.dtt;*.eff;*.evn;*.eft;\0";
}

void DatFileNode::LoadFile() {
	BinaryReader reader(fileData, false);
	reader.Seek(0x4);
	if (reader.ReadUINT32() > 1000000) {
		std::string logName = fileName;
		logName.pop_back();
		CTDLog::Log::getInstance().LogNote("Crueler has decided that " + logName + " is a big endian file! (Xbox 360/PS3)");
		fileIsBigEndian = true;
		reader.SetEndianess(true);
	}
	reader.Seek(0x4);
	unsigned int FileCount = reader.ReadUINT32();
	unsigned int PositionsOffset = reader.ReadUINT32();
	reader.Skip(sizeof(unsigned int)); // TODO: ExtensionsOffset
	unsigned int NamesOffset = reader.ReadUINT32();
	unsigned int SizesOffset = reader.ReadUINT32();
	reader.Skip(sizeof(unsigned int)); // TODO: HashMapOffset

	reader.Seek(PositionsOffset);
	std::vector<int> offsets;
	for (unsigned int f = 0; f < FileCount; f++) {
		offsets.push_back(reader.ReadUINT32());
	}

	reader.Seek(NamesOffset);
	int nameLength = reader.ReadUINT32();
	std::vector<std::string> names;
	for (unsigned int f = 0; f < FileCount; f++) {
		std::string temp_name = reader.ReadString(nameLength);
		temp_name.erase(std::remove(temp_name.begin(), temp_name.end(), '\0'), temp_name.end());
		names.push_back(temp_name);
	}

	reader.Seek(SizesOffset);
	std::vector<int> sizes;
	for (unsigned int f = 0; f < FileCount; f++) {
		sizes.push_back(reader.ReadUINT32());
	}

	for (unsigned int f = 0; f < FileCount; f++) {
		reader.Seek(offsets[f]);
		FileNode* childNode = HelperFunction::LoadNode(names[f], reader.ReadBytes(sizes[f]), fileIsBigEndian, fileIsBigEndian);
		childNode->parent = this;
		if (childNode) {
			children.push_back(childNode);
		}
	}
}

void DatFileNode::SaveFile() {
	std::cout << "Saving DAT file: " << fileName << std::endl;

	auto start_noio = std::chrono::high_resolution_clock::now();

	int longestName = 0;

	for (FileNode* child : children) {
		child->SaveFile();
		if (child->fileName.length() > longestName) {
			longestName = static_cast<int>(child->fileName.length() + 1);
		}
	}

	CRC32 crc32;

	std::vector<std::string> fileNames;
	for (FileNode* node : children) {
		fileNames.push_back(node->fileName);
	}

	int shift = std::min(31, 32 - IntLength(static_cast<int>(fileNames.size())));
	int bucketSize = 1 << (31 - shift);

	std::vector<short> bucketTable(bucketSize, -1);

	std::vector<std::pair<int, short>> hashTuple;
	for (int i = 0; i < fileNames.size(); ++i) {
		//int hashValue = crc32.HashToUInt32(fileNames[i]) & 0x7FFFFFFF;
		int hashValue = ComputeHash(fileNames[i], crc32);
		hashTuple.push_back({ hashValue, static_cast<short>(i) });
	}

	// Sort the hash tuples based on shifted hash values
	std::sort(hashTuple.begin(), hashTuple.end(), [shift](const std::pair<int, short>& a, const std::pair<int, short>& b) {
		return (a.first >> shift) < (b.first >> shift);
		});

	// Populate bucket table with the first unique index for each bucket
	for (int i = 0; i < fileNames.size(); ++i) {
		int bucketIndex = hashTuple[i].first >> shift;
		if (bucketTable[bucketIndex] == -1) {
			bucketTable[bucketIndex] = static_cast<short>(i);
		}
	}

	// Create the result object with the hash data
	HashDataContainer hashData;
	hashData.Shift = shift;
	hashData.Offsets = bucketTable;
	hashData.Hashes.reserve(hashTuple.size());
	hashData.Indices.reserve(hashTuple.size());

	for (const auto& tuple : hashTuple) {
		hashData.Hashes.push_back(tuple.first);
		hashData.Indices.push_back(tuple.second);
	}

	hashData.StructSize = static_cast<int>(4 + 2 * bucketTable.size() + 4 * hashTuple.size() + 2 * hashTuple.size());

	BinaryWriter* writer = new BinaryWriter();
	writer->WriteString("DAT");
	writer->WriteByteZero();
	int fileCount = static_cast<int>(children.size());

	int positionsOffset = 0x20;
	int extensionsOffset = positionsOffset + 4 * fileCount;
	int namesOffset = extensionsOffset + 4 * fileCount;
	int sizesOffset = namesOffset + (fileCount * longestName) + 6;
	int hashMapOffset = sizesOffset + 4 * fileCount;

	writer->WriteUINT32(fileCount);
	writer->WriteUINT32(positionsOffset);
	writer->WriteUINT32(extensionsOffset);
	writer->WriteUINT32(namesOffset);
	writer->WriteUINT32(sizesOffset);
	writer->WriteUINT32(hashMapOffset);
	writer->WriteUINT32(0);

	for (FileNode* child : children) {
		(void)child; // TODO: If child isn't gonna be used in a future patch, remove it entirely instead of discarding.
		writer->WriteUINT32(0);
	}

	for (FileNode* child : children) {
		writer->WriteString(child->fileExtension);
		writer->WriteByteZero();
	}

	writer->WriteUINT32(longestName);
	for (FileNode* child : children) {
		writer->WriteString(child->fileName);
		for (int i = 0; i < longestName - child->fileName.length(); ++i) {
			writer->WriteByteZero();
		}
	}
	// Pad
	writer->WriteINT16(0);

	for (FileNode* child : children) {
		writer->WriteUINT32(static_cast<uint32_t>(child->fileData.size()));
	}

	// Prepare for hash writing
	writer->WriteUINT32(hashData.Shift);
	writer->WriteUINT32(16);
	writer->WriteUINT32(16 + static_cast<uint32_t>(hashData.Offsets.size()) * 2);
	writer->WriteUINT32(16 + static_cast<uint32_t>(hashData.Offsets.size()) * 2 + static_cast<uint32_t>(hashData.Hashes.size()) * 4);

	for (int i = 0; i < hashData.Offsets.size(); i++)
		writer->WriteINT16(hashData.Offsets[i]);

	for (int i = 0; i < fileCount; i++)
		writer->WriteINT32(hashData.Hashes[i]);

	for (int i = 0; i < fileCount; i++)
		writer->WriteINT16(hashData.Indices[i]);

	std::vector<int> offsets;
	for (FileNode* child : children) {
		(void)child; // TODO: If child isn't gonna be used in a future patch, remove it entirely instead of discarding.

		int targetPosition = HelperFunction::Align(static_cast<int>(writer->Tell()), 1024);
		int padding = targetPosition - static_cast<int>(writer->GetData().size());
		if (padding > 0) { // TODO: Replace this with an skip function
			std::vector<char> zeroPadding(padding, 0);
			writer->WriteBytes(zeroPadding);
		}
		offsets.push_back(static_cast<int>(writer->Tell()));
		writer->WriteBytes(child->fileData);
	}

	writer->Seek(positionsOffset);
	for (int i = 0; i < fileCount; i++) {
		writer->WriteUINT32(offsets[i]);
	}

	fileData = writer->GetData();

	auto finish = std::chrono::high_resolution_clock::now();
	auto millisecondsnoio = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start_noio);

	CTDLog::Log::getInstance().LogNote(fileIcon + " Rebuilt in " + std::to_string(millisecondsnoio.count()) + "ms: " + fileName);
}
