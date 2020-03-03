//All the text for the program
//in different languages
//will go here

//https://docs.microsoft.com/en-us/windows/desktop/intl/creating-a-multilingual-user-interface-application

#define ENGLISH 0
#define SPANISH 1

//notification text

const wchar_t *font_failed_T[] = {
	L"Failed to create font for application",
	L"No se pudo crear la fuente del texto para la aplicacion"
};

const wchar_t *error_T[] = {
	L"Error",
	L"Error"
};

const wchar_t *nothing_to_remove_T[] = {
	L"Found nothing to remove",
	L"No se encontró nada para borrar"
};

const wchar_t *add_chars_T[] = {
	L"Please add Initial and Final character",
	L"Por favor agregue caracter inicial y final"
};

const wchar_t *done_T[] = {
	L"   Done   ",
	L"   Listo   "
};

const wchar_t *save_failed_T[] = {
	L"Failed to save the file, please try again",
	L"No se pudo guardar el archivo, vuelva a intentarlo"
};

const wchar_t *file_filter_T[] = {
	L"All files(*.*)\0*.*\0Srt files(*.srt)\0*.srt\0\0",
	L"Todos(*.*)\0*.*\0Srt(*.srt)\0*.srt\0\0"
};

const wchar_t *file_not_existent_T[] = {
	L"The file doesn´t exist",
	L"El archivo no existe"
};

const wchar_t *multi_file_drop_T[] = { //INFO: now this will be supported
	L"Dropping multiple files is not supported",
	L"No está soportado recibir multiples archivos"
};

const wchar_t *folder_drop_T[] = {
	L"Dropping folders is not supported",
	L"No está soportado recibir carpetas"
};

const wchar_t *file_drop_failed_T[] = {
	L"Couldn't retrieve the file",
	L"No se pudo obtener el archivo"
};

const wchar_t *file_comment_found[] = {
	L"Found a comment with: ",
	L"Encontré un comentario con: "
};

const wchar_t *file_comment_not_found[] = {
	L"Didn´t find any comments",
	L"No encontré ningún comentario"
};

const wchar_t *exe_path_failed_T[] = { //INFO: this wont be used anymore, we go to appdata
	L"Couldn't get the path for the executable.\nSaving information to C:\\Temp",
	L"No se pudo obtener la dirección del ejecutable.\nGuardando información en C:\\Temp"
};

//menu text

const wchar_t *file_T[] = {
	L"File",
	L"Archivo"
};

const wchar_t *open_T[] = {
	L"Open		Ctrl+O",
	L"Abrir			Ctrl+O" //TODO(fran): hmm, should the accelerator table change too?
};

const wchar_t *save_T[] = {
	L"Save		Ctrl+S",
	L"Guardar		Ctrl+S"
};

const wchar_t *save_as_T[] = {
	L"Save As	Ctrl+A",
	L"Guardar Como	Ctrl+A"
};

const wchar_t *backup_T[] = {
	L"Backup",
	L"Guardar Copia"
};

const wchar_t *language_T[] = {
	L"Language",
	L"Idioma"
};

//control text

const wchar_t *remove_comment_with_T[] = {
	L"Remove comment with:",
	L"Borrar comentario con:"
};

const wchar_t *brackets_T[] = {
	L"Brackets [ ]",
	L"Llaves [ ]"
};

const wchar_t *parenthesis_T[] = {
	L"Parenthesis ( )",
	L"Parentesis ( )"
};

const wchar_t *braces_T[] = {
	L"Braces { }",
	L"Corchetes { }"
};

const wchar_t *other_T[] = {
	L"Other",
	L"Otro"
};

const wchar_t *initial_char_T[] = {
	L"Initial character:",
	L"Caracter inicial:" //primer caracter
};

const wchar_t *final_char_T[] = {
	L"Final character:",
	L"Caracter final:" //ultimo caracter
};

const wchar_t *remove_T[] = {
	L"Remove",
	L"Borrar"
};