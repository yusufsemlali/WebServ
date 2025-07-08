#pragma once

typedef struct s_error
{
	int		code;
	char	*description;
	char	*file_name;
	size_t	line_number;
	char	*problem;
	char	*hint;
}			t_error;

// error macros go here

void		set_error(t_data *data, char *desc, char *pr, char *hint);
void		check_errors(t_data *data);

