#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>

#include <pe_factory.h>
#include <pe_32_64.h>
#include <pe_resource_manager.h>

#ifdef PE_BLISS_WINDOWS
#include "lib.h"
#endif

#include "structs.h"
#include "morph_parametrs.h"
#include "../unpacker/parameters.h"
#include "unpacker.h"

using namespace pe_bliss;

/*
1) Запомнаем данные секций
2) Запоминаем dll и адреса используемых функций
3) Просмотр релокаций
3) Записываем imports
4) Записываем секции
5) Запись релокаций
*/


void* encXOR(unsigned char* pKey, DWORD dwKeyLen, std::string* pData, DWORD dwDataLen) {
	if (!pKey || dwKeyLen == 0 || dwDataLen == 0) {
		return nullptr;
	}

	for (DWORD i = 0; i < dwDataLen; ++i) {
		(*pData)[i] = (*pData)[i] ^ pKey[i % dwKeyLen];
	}
	return NULL;
}


void* encRC4(unsigned char* pKey, DWORD dwKeyLen, std::string* pData, DWORD dwDataLen) {
	if (!pKey || dwKeyLen == 0 || dwDataLen == 0) {
		return nullptr;
	}

	unsigned char S[256];
	unsigned char temp;
	int i, j = 0, k;

	// Инициализация массива S
	for (i = 0; i < 256; ++i) {
		S[i] = i;
	}

	// Перемешивание массива S на основе ключа
	for (i = 0; i < 256; ++i) {
		j = (j + S[i] + pKey[i % dwKeyLen]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
	}

	// Шифрование данных
	i = j = 0;
	for (DWORD idx = 0; idx < dwDataLen; ++idx) {
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;

		// Обмен значениями S[i] и S[j]
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;

		// Генерация псевдослучайного байта и его XOR с данными
		(*pData)[idx] ^= S[(S[i] + S[j]) % 256];
	}
	return NULL;
}



void replace_magic_number(std::string& data, DWORD magic, DWORD value) {
	for (size_t i = 0; i < data.size() - sizeof(DWORD); ++i) {
		if (*reinterpret_cast<DWORD*>(&data[i]) == magic) {
			*reinterpret_cast<DWORD*>(&data[i]) = value;
		}
	}
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: simple_pe_packer.exe PE_FILE" << std::endl;
		return 0;
	}

	std::ifstream file(argv[1], std::ios::in | std::ios::binary);
	if (!file)
	{
		std::cout << "Cannot open " << argv[1] << std::endl;
		return -1;
	}

	try
	{
		pe32 image(file, false, false);
		{
			std::cout << "Entropy of sections: ";
			double entropy = image.calculate_entropy();
			std::cout << entropy << std::endl;
			if (entropy > 6.8)
			{
				std::cout << "File has already been packed!" << std::endl;
				return -1;
			}
		}


		std::cout << "Get sections..." << std::endl;
		const pe_base::section_list& sections = image.get_image_sections();
		if (sections.empty())
		{
			std::cout << "File has no sections!" << std::endl;
			return -1;
		}

		packed_file_info basic_info = { 0 };

		basic_info.number_of_sections = sections.size();

		basic_info.original_import_directory_rva = image.get_directory_rva(IMAGE_DIRECTORY_ENTRY_IMPORT);
		basic_info.original_import_directory_size = image.get_directory_size(IMAGE_DIRECTORY_ENTRY_IMPORT);

		basic_info.original_resource_directory_rva = image.get_directory_rva(IMAGE_DIRECTORY_ENTRY_RESOURCE);
		basic_info.original_resource_directory_size = image.get_directory_size(IMAGE_DIRECTORY_ENTRY_RESOURCE);

		basic_info.original_relocation_directory_rva = image.get_directory_rva(IMAGE_DIRECTORY_ENTRY_BASERELOC);
		basic_info.original_relocation_directory_size = image.get_directory_size(IMAGE_DIRECTORY_ENTRY_BASERELOC);
		basic_info.original_entry_point = image.get_ep();
		basic_info.total_virtual_size_of_sections = image.get_size_of_image();


		std::cout << "Reading sections..." << std::endl;
		std::string packed_sections_info;
		packed_sections_info.resize(sections.size() * sizeof(packed_section));
		std::string raw_section_data;
		unsigned long current_section = 0;
		for (pe_base::section_list::const_iterator it = sections.begin(); it != sections.end(); ++it, ++current_section)
		{
			const pe_base::section& s = *it;

			{
				packed_section& info
					= reinterpret_cast<packed_section&>(packed_sections_info[current_section * sizeof(packed_section)]);

				//Характеристики секции
				info.characteristics = s.get_characteristics();
				//Указатель на файловые данные
				info.pointer_to_raw_data = s.get_pointer_to_raw_data();
				//Размер файловых данных
				info.size_of_raw_data = s.get_size_of_raw_data();
				//Относительный виртуальный адрес секции
				info.virtual_address = s.get_virtual_address();
				//Виртуальный размер секции
				info.virtual_size = s.get_virtual_size();

				//Копируем имя секции
				memset(info.name, 0, sizeof(info.name));
				memcpy(info.name, s.get_name().c_str(), s.get_name().length());
				std::cout << "	Reading sections: " << s.get_name() << std::endl;
			}

			if (s.get_raw_data().empty())
				continue;

			raw_section_data = s.get_raw_data();

			if (raw_section_data.empty())
			{
				std::cout << "All sections of PE file are empty!" << std::endl;
				return -1;
			}

			packed_sections_info += raw_section_data;
		}

		
		/* Шифование */
		char key[9] = "deedbeef";
		memcpy(basic_info.key, key, 9);


		switch (enc_algo) {
		case 0x01:
			encXOR(reinterpret_cast<unsigned char*>(key), 9, &packed_sections_info, packed_sections_info.size());
			break;
		case 0x02:
			//encAES(reinterpret_cast<unsigned char*>(key), 9, &packed_sections_info, packed_sections_info.size());
			break;
		case 0x3:
			encRC4(reinterpret_cast<unsigned char*>(key), 9, &packed_sections_info, packed_sections_info.size());
			break;
		default:
			// Обработка случая, когда значение enc_algo некорректно
			throw std::runtime_error("Unknown encryption algorithm specified.");
		}

		//encXOR(reinterpret_cast<unsigned char*>(key), 9, &packed_sections_info, packed_sections_info.size());


		std::cout << "Make section .rsrc" << std::endl;
		pe_base::section new_section;
		new_section.set_name(".rsrc");
		new_section.readable(true).writeable(true).executable(true);
		std::string& out_buf = new_section.get_raw_data();


		basic_info.size_of_packed_data = packed_sections_info.size();
		out_buf =
			std::string(reinterpret_cast<const char*>(&basic_info), sizeof(basic_info))
			+ packed_sections_info;
		std::cout << packed_sections_info.size() << std::endl;


		std::auto_ptr<pe_base::tls_info> tls;
		if (image.has_tls())
		{
			std::cout << "Reading TLS..." << std::endl;
			tls.reset(new pe_base::tls_info(image.get_tls_info()));
		}

		pe_base::exported_functions_list exports;
		pe_base::export_info exports_info;
		if (image.has_exports())
		{
			std::cout << "Reading exports..." << std::endl;
			exports = image.get_exported_functions(exports_info);
		}


		{
			const pe_base::section& first_section = image.get_image_sections().front();
			new_section.set_virtual_address(first_section.get_virtual_address());
			const pe_base::section& last_section = image.get_image_sections().back();

			DWORD total_virtual_size =
				last_section.get_virtual_address()
				+ pe_base::align_up(last_section.get_virtual_size(), image.get_section_alignment())
				- first_section.get_virtual_address();

			pe_base::resource_directory new_root_dir;
			if (image.has_resources())
			{
				std::cout << "Repacking resources..." << std::endl;
				pe_base::resource_directory root_dir = image.get_resources();

				pe_resource_viewer res(root_dir);
				pe_resource_manager new_res(new_root_dir);
				try
				{
					pe_resource_viewer::resource_id_list icon_id_list(res.list_resource_ids(pe_resource_viewer::resource_icon_group));
					pe_resource_viewer::resource_name_list icon_name_list(res.list_resource_names(pe_resource_viewer::resource_icon_group));

					if (!icon_name_list.empty())
					{
						new_res.add_icon(
							res.get_icon_by_name(icon_name_list[0]),
							icon_name_list[0],
							res.list_resource_languages(pe_resource_viewer::resource_icon_group, icon_name_list[0]).at(0));
					}
					else if (!icon_id_list.empty())
					{
						new_res.add_icon(
							res.get_icon_by_id(icon_id_list[0]),
							icon_id_list[0],
							res.list_resource_languages(pe_resource_viewer::resource_icon_group, icon_id_list[0]).at(0));
					}
				}
				catch (const pe_exception&) {}

				try
				{
					pe_resource_viewer::resource_id_list manifest_id_list(res.list_resource_ids(pe_resource_viewer::resource_manifest));
					if (!manifest_id_list.empty())
					{
						new_res.add_resource(
							res.get_resource_data_by_id(pe_resource_viewer::resource_manifest, manifest_id_list[0]).get_data(),
							pe_resource_viewer::resource_manifest,
							manifest_id_list[0],
							res.list_resource_languages(pe_resource_viewer::resource_manifest, manifest_id_list[0]).at(0)
						);
					}
				}
				catch (const pe_exception&) {}

				try
				{
					pe_resource_viewer::resource_id_list version_info_id_list(res.list_resource_ids(pe_resource_viewer::resource_version));
					if (!version_info_id_list.empty())
					{
						new_res.add_resource(
							res.get_resource_data_by_id(pe_resource_viewer::resource_version, version_info_id_list[0]).get_data(),
							pe_resource_viewer::resource_version,
							version_info_id_list[0],
							res.list_resource_languages(pe_resource_viewer::resource_version, version_info_id_list[0]).at(0)
						);
					}
				}
				catch (const pe_exception&) {}
			}

			image.get_image_sections().clear();
			image.realign_file(0x200);

			pe_base::section& added_section = image.add_section(new_section);
			image.set_section_virtual_size(added_section, total_virtual_size);

			std::cout << "Creating imports..." << std::endl;

			pe_base::import_library kernel32;
			kernel32.set_name("kernel32.dll");

			pe_base::imported_function func;
			func.set_name("LoadLibraryA");
			kernel32.add_import(func);

			func.set_name("GetProcAddress");
			kernel32.add_import(func);

			DWORD load_library_address_rva = pe_base::rva_from_section_offset(added_section,
				offsetof(packed_file_info, load_library_a));

			kernel32.set_rva_to_iat(load_library_address_rva);

			pe_base::imported_functions_list imports;
			imports.push_back(kernel32);


			pe_base::import_rebuilder_settings settings;
			settings.build_original_iat(false);
			settings.save_iat_and_original_iat_rvas(true, true);

			settings.set_offset_from_section_start(added_section.get_raw_data().size());

			if (!new_root_dir.get_entry_list().empty())
				settings.enable_auto_strip_last_section(false);

			image.rebuild_imports(imports, added_section, settings);

			if (!new_root_dir.get_entry_list().empty())
				image.rebuild_resources(new_root_dir, added_section, added_section.get_raw_data().size());

			if (tls.get())
			{
				packed_file_info* info = reinterpret_cast<packed_file_info*>(&added_section.get_raw_data()[0]);
				info->original_tls_index_rva = tls->get_index_rva();
				if (!tls->get_tls_callbacks().empty())
					info->original_rva_of_tls_callbacks = tls->get_callbacks_rva();
					tls->set_index_rva(pe_base::rva_from_section_offset(added_section, offsetof(packed_file_info, tls_index)));
			}
		}



		DWORD first_callback_offset = 0;
		{

			pe_base::section unpacker_section;
			unpacker_section.set_name(".text");
			unpacker_section.readable(true).executable(true).writeable(true);
			{
				std::cout << "Writing unpacker stub, size = " << sizeof(unpacker_data) << " bytes" << std::endl;

				std::string& unpacker_section_data = unpacker_section.get_raw_data();
				
				unpacker_section_data = std::string(reinterpret_cast<const char*>(unpacker_data), sizeof(unpacker_data));

				std::cout << "Image base: " << image.get_image_base_32() << std::endl;
				std::cout << "Image base no fixup: " << image.get_image_base_32() << std::endl;
				std::cout << "RVA of first section: " << image.get_image_sections().at(0).get_virtual_address() << std::endl;

				//*reinterpret_cast<DWORD*>(&unpacker_section_data[original_image_base_offset]) = image.get_image_base_32();
				//*reinterpret_cast<DWORD*>(&unpacker_section_data[original_image_base_no_fixup_offset]) = image.get_image_base_32();
				//*reinterpret_cast<DWORD*>(&unpacker_section_data[rva_of_first_section_offset]) = image.get_image_sections().at(0).get_virtual_address();
				replace_magic_number(unpacker_section_data, 0x11111111, image.get_image_base_32());
				replace_magic_number(unpacker_section_data, 0x22222222, image.get_image_sections().at(0).get_virtual_address());
				replace_magic_number(unpacker_section_data, 0x33333333, image.get_image_base_32());
			
			}

			pe_base::section& unpacker_added_section = image.add_section(unpacker_section);

			
			if (tls.get() || image.has_exports() || image.has_reloc())
			{
				unpacker_added_section.get_raw_data().resize(sizeof(unpacker_data));
			}

				if (tls.get())
				{
					std::cout << "Rebuilding TLS..." << std::endl;



					std::string& data = unpacker_added_section.get_raw_data();
					DWORD directory_pos = data.size();

					data.resize(data.size() + sizeof(IMAGE_TLS_DIRECTORY32) + sizeof(DWORD));
						if (!tls->get_tls_callbacks().empty())
						{
							first_callback_offset = data.size();
							data.resize(data.size() + sizeof(DWORD) * (tls->get_tls_callbacks().size() + 1));
							*reinterpret_cast<DWORD*>(&data[first_callback_offset]) =
							image.rva_to_va_32(pe_base::rva_from_section_offset(unpacker_added_section, empty_tls_callback_offset));

							tls->set_callbacks_rva(pe_base::rva_from_section_offset(unpacker_added_section, first_callback_offset));
							reinterpret_cast<packed_file_info*>(&image.get_image_sections().at(0).get_raw_data()[0])->new_rva_of_tls_callbacks = tls->get_callbacks_rva();
						}
						else
						{
								tls->set_callbacks_rva(0);
						}

						tls->clear_tls_callbacks();



					tls->set_raw_data_start_rva(pe_base::rva_from_section_offset(unpacker_added_section, data.size()));

					tls->recalc_raw_data_end_rva();

					image.rebuild_tls(*tls, unpacker_added_section, directory_pos, false, false, pe_base::tls_data_expand_raw, true, false);



					unpacker_added_section.get_raw_data() += tls->get_raw_data();
					image.set_section_virtual_size(unpacker_added_section, data.size() + tls->get_size_of_zero_fill());

					if (!image.has_reloc() && !image.has_exports())
						pe_base::strip_nullbytes(unpacker_added_section.get_raw_data());

					image.prepare_section(unpacker_added_section);
				}

				image.set_ep(image.rva_from_section_offset(unpacker_added_section, 0));
		}

		if (image.has_reloc())
		{
			std::cout << "Creating relocations..." << std::endl;


			pe_base::relocation_table_list reloc_tables;
			pe_base::relocation_table table;

			pe_base::section& unpacker_section = image.get_image_sections().at(1);

			table.set_rva(unpacker_section.get_virtual_address());
			table.add_relocation(pe_base::relocation_entry(original_image_base_offset, IMAGE_REL_BASED_HIGHLOW));
			if (tls.get())
			{
				DWORD tls_directory_offset = image.get_directory_rva(IMAGE_DIRECTORY_ENTRY_TLS)
					- image.section_from_directory(IMAGE_DIRECTORY_ENTRY_TLS).get_virtual_address();


				table.add_relocation(pe_base::relocation_entry(static_cast<WORD>(tls_directory_offset + offsetof(IMAGE_TLS_DIRECTORY32, StartAddressOfRawData)), IMAGE_REL_BASED_HIGHLOW));
				table.add_relocation(pe_base::relocation_entry(static_cast<WORD>(tls_directory_offset + offsetof(IMAGE_TLS_DIRECTORY32, EndAddressOfRawData)), IMAGE_REL_BASED_HIGHLOW));
				table.add_relocation(pe_base::relocation_entry(static_cast<WORD>(tls_directory_offset + offsetof(IMAGE_TLS_DIRECTORY32, AddressOfIndex)), IMAGE_REL_BASED_HIGHLOW));

				if (first_callback_offset)
				{
					table.add_relocation(pe_base::relocation_entry(static_cast<WORD>(tls_directory_offset + offsetof(IMAGE_TLS_DIRECTORY32, AddressOfCallBacks)), IMAGE_REL_BASED_HIGHLOW));
					table.add_relocation(pe_base::relocation_entry(static_cast<WORD>(first_callback_offset), IMAGE_REL_BASED_HIGHLOW));
				}
			}

			reloc_tables.push_back(table);
			image.rebuild_relocations(reloc_tables, unpacker_section, unpacker_section.get_raw_data().size(), true, !image.has_exports());
		}


		if (image.has_exports())
		{
			std::cout << "Repacking exports..." << std::endl;

			pe_base::section& unpacker_section = image.get_image_sections().at(1);
			image.rebuild_exports(exports_info, exports, unpacker_section, unpacker_section.get_raw_data().size());
		}
		
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_IAT);
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_SECURITY);
		image.remove_directory(IMAGE_DIRECTORY_ENTRY_DEBUG);

		
		image.strip_stub_overlay();


		std::string base_file_name(argv[1]);
		std::string dir_name;
		std::string::size_type slash_pos;
		if ((slash_pos = base_file_name.find_last_of("/\\")) != std::string::npos)
		{
			dir_name = base_file_name.substr(0, slash_pos + 1);
			base_file_name = base_file_name.substr(slash_pos + 1);
		}

		base_file_name = dir_name + "packed_" + base_file_name;

		std::ofstream new_pe_file(base_file_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (!new_pe_file)
		{
			std::cout << "Cannot create " << base_file_name << std::endl;
			return -1;
		}


		image.rebuild_pe(new_pe_file, true, false);
		std::cout << "Packed image was saved to " << base_file_name << std::endl;
	}
	catch (const pe_exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

