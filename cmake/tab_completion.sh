#. ~/.bashrc // why source not usable?
while read line
do
  if [[ $line == "SVAR_COMPLETION_ENABLED=ON" ]];then
    SVAR_COMPLETION_ENABLED=ON
  fi
done < ~/.bashrc


if [ -z "$SVAR_COMPLETION_ENABLED" ];then
echo '
# svar tab completion support
function_svar_complete()
{
    COMPREPLY=()
    local cur=${COMP_WORDS[COMP_CWORD]};
    local com=${COMP_WORDS[COMP_CWORD-1]};
    local can=$(${COMP_WORDS[*]:0:COMP_CWORD} -help -complete_function_request)
    local reg="-.+"
    if [[ $com =~ $reg ]];then
      COMPREPLY=($(compgen -df -W "$can" -- $cur))
    else
      COMPREPLY=($(compgen -W "$can" -- $cur))
    fi
}

complete -F function_svar_complete "svar"
SVAR_COMPLETION_ENABLED=ON
'>> ~/.bashrc
else
  echo "svar tab completion has already supported."
fi


