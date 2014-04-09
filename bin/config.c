#include "config.h"
/*
 * ** Slave
 * ** file: slave_conf.c
 * ** time: 2012.7.18
 * */



/******************************************************************************
 *  *                                                                            *
 *   * Function: slave_ltrim                                                        *
 *    *                                                                            *
 *     * Purpose: Strip characters from the beginning of a string                   *
 *      *                                                                            *
 *       * Parameters: str - string for processing                                    *
 *        *             charlist - null terminated list of characters                  *
 *         *                                                                            *
 *          * Return value:                                                              *
 *           *                                                                            *
 *            *                                                 *
 *             *                                                                            *
 *              ******************************************************************************/
static void	slave_ltrim(char *str, const char *charlist)
{
	char	*p;

	if (NULL == str || '\0' == *str)
		return;

	for (p = str; '\0' != *p && NULL != strchr(charlist, *p); p++)
		;

	if (p == str)
		return;

	while ('\0' != *p)
		*str++ = *p++;

	*str = '\0';
}


/******************************************************************************
 *  *                                                                            *
 *   * Function: slave_rtrim                                                        *
 *    *                                                                            *
 *     * Purpose: Strip characters from the end of a string                         *
 *      *                                                                            *
 *       * Parameters: str - string for processing                                    *
 *        *             charlist - null terminated list of characters                  *
 *         *                                                                            *
 *          * Return value: number of trimmed characters                                 *
 *           *                                                                            *
 *            * Author: Eugene Grigorjev, Aleksandrs Saveljevs                             *
 *             *                                                                            *
 *              ******************************************************************************/
int	slave_rtrim(char *str, const char *charlist)
{
	char	*p;
	int	count = 0;

	if (NULL == str || '\0' == *str)
		return count;

	for (p = str + strlen(str) - 1; p >= str && NULL != strchr(charlist, *p); p--)
	{
		*p = '\0';
		count++;
	}

	return count;
}



/******************************************************************************
 *  *                                                                            *
 *   * Function: is_uint64_n                                                      *
 *    *                                                                            *
 *     * Purpose: check if the string is 64bit unsigned integer                     *
 *      *                                                                            *
 *       * Parameters: str   - [IN] string to check                                   *
 *        *             n     - [IN] string length or ZBX_MAX_UINT64_LEN               *
 *         *             value - [OUT] a pointer to converted value (optional)          *
 *          *                                                                            *
 *           * Return value:  SUCCEED - the string is unsigned integer                    *
 *            *                FAIL - the string is not a number or overflow               *
 *             *                                                                            *
 *              * Author: Alexander Vladishev                                                *
 *               *                                                                            *
 *                ******************************************************************************/
int	is_uint64_n(const char *str, size_t n, uint64_t *value)
{
	//const uint64_t	max_uint64 = ~(uint64_t)__UINT64_C(0);
	const uint64_t	max_uint64 = ~(uint64_t)(0);
  uint64_t		value_uint64 = 0, c;

	if ('\0' == *str || 0 == n)
		return FAIL;

	while ('\0' != *str && 0 < n--)
	{
		if (0 == isdigit(*str))
			return FAIL;	/* not a digit */

		c = (uint64_t)(unsigned char)(*str - '0');

		if ((max_uint64 - c) / 10 < value_uint64)
			return FAIL;	/* overflow */

		value_uint64 = value_uint64 * 10 + c;

		str++;
	}

	if (NULL != value)
		*value = value_uint64;

	return SUCCEED;
}






/******************************************************************************
 *  *                                                                            *
 *   * Function: str2uint64                                                       *
 *    *                                                                            *
 *     * Purpose: convert string to 64bit unsigned integer                          *
 *      *                                                                            *
 *       * Parameters: str   - string to convert                                      *
 *        *             value - a pointer to converted value                           *
 *         *                                                                            *
 *          * Return value:  SUCCEED - the string is unsigned integer                    *
 *           *                FAIL - otherwise                                            *
 *            *                                                                            *
 *             * Author: Alexander Vladishev                                                *
 *              *                                                                            *
 *               * Comments: the function automatically processes suffixes K, M, G, T         *
 *                *                                                                            *
 *                 ******************************************************************************/
int	str2uint64(char *str, uint64_t *value)
{
	size_t		sz;
	int		ret;
	uint64_t	factor = 1;
	char		c = '\0';

	sz = strlen(str) - 1;

	if (str[sz] == 'K')
	{
		c = str[sz];
		factor = SLAVE_KIBIBYTE;
	}
	else if (str[sz] == 'M')
	{
		c = str[sz];
		factor = SLAVE_MEBIBYTE;
	}
	else if (str[sz] == 'G')
	{
		c = str[sz];
		factor = SLAVE_GIBIBYTE;
	}
	else if (str[sz] == 'T')
	{
		c = str[sz];
		factor = SLAVE_GIBIBYTE;
		factor *= SLAVE_KIBIBYTE;
	}

	if ('\0' != c)
		str[sz] = '\0';

	if (SUCCEED == (ret = is_uint64(str, value)))
		*value *= factor;

	if ('\0' != c)
		str[sz] = c;

	return ret;
}


/******************************************************************************
 *                                                                            *
 * Function: parse_cfg_file                                                   *
 *
 * Parameters: cfg_file - full name of config file                            *
 *                  
 * Return value: SUCCEED - parsed successfully                                *
 *               FAIL - error processing config file                          *
 * Time: 2012.7.19                                                      
 *****************************************************************************/

int parse_cfg_file(const char *cfg_file, struct cfg_line *cfg)
{
	FILE		*file;
  
	int param_valid,lineno,i;
	char		line[MAX_STRING_LEN], *parameter, *value;
	uint64_t var;
	assert(cfg);
	
	if (NULL != cfg_file)
	  { 
		  if (NULL == (file = fopen(cfg_file, "r")))
			 {
				 fclose(file);
				 //slave_log(NULL,P_ERROR,"In func %s :%s\n",__FUNCTION__, "open cfg file fialed!");
				 return -1;
			 }
		
			
    	
   
     for (lineno = 1; NULL != fgets(line, sizeof(line), file); lineno++)
   		{ 
   			slave_ltrim(line, SLAVE_CFG_LTRIM_CHARS);
   			slave_rtrim(line, SLAVE_CFG_RTRIM_CHARS);
   
   			if ('#' == *line || '\0' == *line)
   				continue;
   			parameter = line;
   			if (NULL == (value = strchr(line, '=')))
   				{
   					fclose(file);
   				  //slave_log(NULL,P_ERROR,"invalid entry [%s] (not following \"parameter=value\" notation) in config file [%s], line %d",line, cfg_file, lineno);
   				  return -1;
   				}
   				
   
   			*value++ = '\0';
   
   			slave_rtrim(parameter, SLAVE_CFG_RTRIM_CHARS);
   
   			slave_ltrim(value, SLAVE_CFG_LTRIM_CHARS);
   
   			//slave_log(NULL,P_DEBUG, "cfg: para: [%s] val [%s]", parameter, value);	
   			
   			for (i = 0; '\0' != value[i]; i++)
   			  {
   				  if ('\n' == value[i])
   				    {
   					    value[i] = '\0';
   					    break;
   				    }
   			  }
   			  
   			param_valid = 0;
   			for (i = 0; NULL != cfg[i].parameter; i++)
   			{
   				if (0 != strcmp(cfg[i].parameter, parameter))
   					continue;
   
   				param_valid = 1;
   
   				//slave_log(NULL,P_DEBUG, "accepted configuration parameter: '%s' = '%s'",parameter, value);
   
   				if (TYPE_INT == cfg[i].type)
   				{
   										
   					if (FAIL == str2uint64(value, &var))
   						{
   					     fclose(file);
   				       //slave_log(NULL,P_ERROR," In func %s: str2uint64()fialed",__FUNCTION__);
   				       return -1;
   				    }
   
   					if ((cfg[i].min && var < cfg[i].min) || (cfg[i].max && var > cfg[i].max))
   						{
   					     fclose(file);
   				       //slave_log(NULL,P_ERROR," In func %s: Invalid value ,out of range",__FUNCTION__);
   				       return -1;
   				    }
   
   					*((int *)cfg[i].variable) = (int)var;
   				}
   				else if (TYPE_STRING == cfg[i].type)
   				{
   					/* free previous value memory */
   					char *p = *((char **)cfg[i].variable);
   					if (NULL != p)
   						{
   						  free(p);
   						  p=NULL;
   						}
   
   					*((char **)cfg[i].variable) = strdup(value);
   					//strcpy((char *)cfg[i].variable,value);
   				}
   				
   				else
   					assert(0);
   			}
   			
   
   		}
   	  fclose(file);
   }
   return 0;	 
}
 
