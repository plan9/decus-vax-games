{ Data Corruption means character is dead, or save file was screwed with.
  Keep them guessing as to what is actually wrong.}
[psect(save$code)] procedure data_exception;
      begin
        clear(1,1);
        prt('%BOSS-F-ATTREUSE,Attempted Reuse of a Character',1,1);
        prt('',2,1);
        exit; 
      end;

[psect(save$code)] procedure data_exception1;
      begin
        clear(1,1);
        prt('%BOSS-F-BADCHECK,Data Exceptyion Error,Corruption',1,1);
        prt('',2,1);
        exit; 
      end;
 
 
{ Uses XOR function to encode data }
[psect(save$code)] procedure coder(var line : ntype);
      var
                i1                      : integer;
                i2,i3,i4                : unsigned;
      begin
        for i1 := 1 to length(line) do
          begin
            i2 := uint(ord(line[i1]));
            i3 := uint(randint(256)-1);
            i4 := uxor(i2,i3);
            line[i1] := chr(i4);
          end;
      end;
 
 
{ Encrypts a line of text, complete with a data-check sum }
[psect(save$code)] procedure encrypt(var line : ntype);
      var
        i1,i2           : integer;
        temp            : ntype;
 
      begin
        i2 := 0;
        for i1 := 1 to length(line) do
          i2 := i2 + ord(line[i1]) + i1;
        temp := line;
        writev(line,i2:1,' ',temp);
        coder(line);
      end;
 
 
{ Decrypts a line of text, complete with a data-check sum }
[psect(save$code)] procedure decrypt(var line : ntype);
      var
        i1,i2,i3        : integer;
        temp            : ntype;
        tmp             : char;
 
      begin
        i2 := 0;
        coder(line);
        temp := line;
        readv(temp,i3,tmp,line);
        for i1 := 1 to length(line) do
          i2 := i2 + ord(line[i1]) + i1;
        if (i2 <> i3) then data_exception1;
      end;
 
 
{ Actual save procedure }
[psect(save$code)] function save_char : boolean;
      var
        tot_monsters,tot_treasure     : integer;
        i1,i2,trys                    : integer;
        xfloor                        : unsigned;
        save_seed                     : unsigned;
        fnam,saveline                 : vtype;
        temp                          : packed array [1..70] of char;
        out_rec,title1,title2         : ntype;
        f1                            : text;
        f2                            : file of key_type;
        flag,file_flag                : boolean;
 
      begin
        prt('Enter Filename:',1,1);
        flag := false;
{ Open the user's save file }
        if (get_string(fnam,1,17,60)) then
          begin
            if (length(fnam) = 0) then fnam := 'BOSSCHR.SAV';
            open (f1,FILE_NAME:=fnam,record_length:=1024,ERROR:=continue);
            if (status(f1) <> 0) then
              msg_print('Error creating> ' + fnam)
            else
              flag := true;
          end;
    
    { Make an attempt to open the MASTER file               -JWT-   }
        if (flag) then
          begin
            rewrite(f1,error:=continue);
            priv_switch(1);
            open (f2,file_name:=boss_mas,
                    error:=continue,access_method:=keyed,organization:=indexed,
                    history:=old,sharing:=readwrite);
            if (status(f2) <> 0) then
              begin
                priv_switch(0);
                open (f2,file_name:=boss_mas,
                        error:=continue,access_method:=keyed,
                        organization:=indexed,history:=new,sharing:=readwrite);
              end;
            if (status(f2) <> 0) then
                begin
                  msg_print('Error saving character, contact BOSS Wizard.');
                  close(f1,error:=continue);
                  flag := false;
                end;
          end;
        { Make an attempt to create a unique ID and write to MASTER -RAK-}
        if (flag) then
          begin
            trys := 0;
            file_flag := false;
            repeat
              for i1 := 1 to 70 do temp[i1] := chr(31+randint(95));
              title1 := temp;
              seed := encrypt_seed1;
              encrypt(title1);
              for i1 := 1 to 70 do
                key_rec.file_id[i1] := title1[i1];
              f2^ := key_rec;
              put(f2,error:=continue);
              if (status(f2) <> 0) then
                begin
                  trys := trys + 1;
                  if (trys > 5) then
                    begin
                      file_flag := true;
                      flag := false;
                      msg_print('Error in writing to MASTER.');
                    end;
                end
              else
                file_flag := true;
            until(file_flag);
            close(f2,error:=continue);
            priv_switch(0);
          end;
        { If ID was written to MASTER, continue saving          -RAK-   }

        if (flag) then
          begin
            save_seed := get_seed;
            writev(title2,save_seed:12,' ',temp);
            seed := encrypt_seed2;
            encrypt(title2);
            writeln(f1,title2,error:=continue);
            seed := save_seed;
                { Message to player on what is happening}
            clear(1,1);
            writev(saveline,'Saving character as ',fnam,'...'); 
            prt(saveline,1,1);
            put_qio;
                { Version number of Boss               }
            writev(out_rec,cur_version);
            encrypt(out_rec);
            writeln(f1,out_rec,error:=continue);
 
                { Write out the player record.  }
            with py.misc do
              begin
   writev(out_rec,name); encrypt(out_rec); writeln(f1,out_rec,error:=continue);
   writev(out_rec,race); encrypt(out_rec); writeln(f1,out_rec,error:=continue);
   writev(out_rec,sex);  encrypt(out_rec); writeln(f1,out_rec,error:=continue);
  writev(out_rec,tclass);encrypt(out_rec); writeln(f1,out_rec,error:=continue);
   writev(out_rec,title);encrypt(out_rec); writeln(f1,out_rec,error:=continue);
 
                for i1 := 1 to 5 do
                  begin
                    out_rec := history[i1];
                    encrypt(out_rec);
                    writeln(f1,out_rec,error:=continue);
                  end;
 
                writev(out_rec,char_row:1,' ',char_col:1,' ',
                  pclass:1,' ',prace:1,' ', pskill:1, ' ',
                  age:1,' ',ht:1,' ',wt:1,' ',sc:1,' ',max_exp:1,' ',
                  exp:1,' ',lev:1,' ',max_lev:1,' ',expfact:2:1);
 
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,srh:1,' ',perc:1,' ',stl:1,' ',bth:1,' ',
			bthb:1,' ',mana:1,' ',cmana:1,' ',mhp:1,' ',
			chp:1:1,' ',au:1,' ',ptohit:1,' ',ptodam:1,' ',
			pac:1,' ',ptoac:1,' ',dis_th:1,' ',dis_td:1,' ',
			dis_ac:1,' ',dis_tac:1,' ',disarm:1,' ',
			wierd:1,' ',save:1,' ',hitdie:1);
 
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
              end;

            writev(out_rec,inven_ctr:1,' ',
            inven_weight:1,' ',equip_ctr:1,' ',dun_level:1,' ',
            missle_ctr:1,' ',mon_tot_mult:1,' ',turn:1,' ',day:1,
	                  ' ',town_num:1,' ',defeated_badguy:1,
			  ' ',randes_seed:12);
            encrypt(out_rec);
            writeln(f1,out_rec,error:=continue);
 
 		{Write out the kill list}
	    for i1 := 1 to 400 do
	      begin
	        writev(out_rec,kill_list[i1]:1);
		encrypt(out_rec);
		writeln(f1,out_rec,error:=continue);
	      end;

      		{Write out bank records} 
	    writev(out_rec,act_bal:1,' ',principal:1,' ',payment_count:1,
                  ' ',iod_val:2:1,' ',iol_val:2:1,' ',danny_cut:2:1);
	    encrypt(out_rec);
	    writeln(f1,out_rec,error:=continue);

                { Write out the inventory records.      }
            for i1 := 1 to inven_ctr do
              begin
                writev(out_rec,inventory[i1].tchar,inventory[i1].name);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,inventory[i1].damage);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                with inventory[i1] do
                  writev(out_rec,tval:1,' ',subval:1,' ',weight:1,' ',
                        number:1,' ',tohit:1,' ',todam:1,' ',ac:1,' ',
                        toac:1,' ',p1:1,' ',flags:1,' ',level:1,' ',cost:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
              end;
 
                { Write out the equipment records.      }
            for i1 := 23 to equip_max do
              begin
                writev(out_rec,equipment[i1].tchar,equipment[i1].name);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,equipment[i1].damage);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                with equipment[i1] do
                  writev(out_rec,tval:1,' ',subval:1,' ',weight:1,' ',
                        number:1,' ',tohit:1,' ',todam:1,' ',ac:1,' ',
                        toac:1,' ',p1:1,' ',flags:1,' ',level:1,' ',cost:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
              end;
 
            with py.stat do
              begin
                writev(out_rec,str:1,' ',cstr:1,' ',dex:1,' ',cdex:1,' ',
                        con:1,' ',ccon:1,' ',int:1,' ',cint:1,' ',wis:1,' ',
                        cwis:1,' ',chr:1,' ',cchr:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
              end;
 
            with py.flags do
              begin
                writev(out_rec,status:1,' ',blind:1,' ',confused:1,' ',
                        food:1,' ',food_digested:1,' ',protection:1,' ',
                        speed:1,' ',afraid:1,' ',
                        poisoned:1,' ',see_inv:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,fast:1,' ',slow:1,' ',protevil:1,' ',
                        teleport:1,' ',free_act:1,' ',slow_digest:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,aggravate:1,' ',sustain_str:1,' ',
                        sustain_int:1,' ',sustain_wis:1,' ',sustain_con:1,' ',
                        sustain_dex:1,' ',sustain_chr:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,fire_resist:1,' ',cold_resist:1,' ',
                        acid_resist:1,' ',regenerate:1,' ',lght_resist:1,' ',
                        ffall:1,' ',confuse_monster:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
 
                writev(out_rec,image:1,' ',invuln:1,' ',hero:1,' ',
                        shero:1,' ',blessed:1,' ',resist_heat:1,' ',
			resist_cold:1,' ',ring_blast:1,' ',special:1,' ',
			petrified:1,' ',diseased:1,' ',detect_inv:1,' ',
			word_recall:1,' ',see_infra:1,' ',tim_infra:1);
                encrypt(out_rec);
                writeln(f1,out_rec,error:=continue);
              end;
 
            for i1 := 1 to max_skills do
	      with skill_list[i1] do
                begin
                  writev(out_rec,sname);
                  encrypt(out_rec);
                  writeln(f1,out_rec,error:=continue);

                  writev(out_rec,slevel:2,' ',smana:2,' ',
		         sexp:3,' ',sfail:2,' ',learned:5);
                  encrypt(out_rec);
                  writeln(f1,out_rec,error:=continue);
                end;

           { Write the important dungeon info and floor    -RAK-   }
            begin
              writev(out_rec,cur_height:1,' ',cur_width:1,' ',
                max_panel_rows:1,' ',max_panel_cols:1);
              encrypt(out_rec);
              writeln(f1,out_rec,error:=continue);
 
                { Save the floor        }
              tot_treasure := 0;
              for i1 := 1 to cur_height do
                begin
                  out_rec := pad(' ',' ',cur_width);
                  for i2 := 1 to cur_width do
                    begin
                      with cave[i1,i2] do
                        begin
                          xfloor := fval;
                          if (fopen) then
                            xfloor := uor(xfloor,%X'10');
                          if (pl) then
                            xfloor := uor(xfloor,%X'20');
                          if (fm) then
                            xfloor := uor(xfloor,%X'40');
                          out_rec[i2] := chr(xfloor);
                          if (tptr > 0) then
                            tot_treasure := tot_treasure + 1;
                        end;
                    end;
                  encrypt(out_rec);
                  writeln(f1,out_rec,error:=continue);
                end;
 
                { Save the Treasure List                }
              writev(out_rec,tot_treasure:1);
              encrypt(out_rec);
              writeln(f1,out_rec,error:=continue);
              for i1 := 1 to cur_height do
                for i2 := 1 to cur_width do
                  if (cave[i1,i2].tptr > 0) then
                    with t_list[cave[i1,i2].tptr] do
                      begin
                        writev(out_rec,i1:1,' ',i2:1);
                        encrypt(out_rec);
                        writeln(f1,out_rec,error:=continue);
 
                        writev(out_rec,tchar,name);
                        encrypt(out_rec);
                        writeln(f1,out_rec,error:=continue);
 
                        writev(out_rec,damage);
                        encrypt(out_rec);
                        writeln(f1,out_rec,error:=continue);
 
                        writev(out_rec,tval:1,' ',subval:1,' ',weight:1,' ',
                          number:1,' ',tohit:1,' ',todam:1,' ',ac:1,' ',
                          toac:1,' ',p1:1,' ',flags:1,' ',level:1,' ',cost:1);
                        encrypt(out_rec);
                        writeln(f1,out_rec,error:=continue);
                      end;
 
                { Save identified list                  }
              out_rec := '';
              for i1 := 1 to max_objects do
                begin
                  if (object_ident[i1]) then
                    out_rec := out_rec + 'T'
                  else
                    out_rec := out_rec + 'F';
                end;
              encrypt(out_rec);
              writeln(f1,out_rec,error:=continue);
 
                { Save the Monster List                 }
              i1 := muptr;
              tot_monsters := 0;
              if (i1 > 0) then
                repeat
                  tot_monsters := tot_monsters + 1;
                  with m_list[i1] do
                    i1 := nptr;
                until (i1 = 0);
              writev(out_rec,tot_monsters:1);
              encrypt(out_rec);
              writeln(f1,out_rec,error:=continue);
              i1 := muptr;
              if (i1 > 0) then
                repeat
                  with m_list[i1] do
                    begin
                      writev(out_rec,fy:1,' ',fx:1,' ',mptr:1,' ',hp:1,
                        ' ',cspeed:1,' ',csleep:1,' ',cdis:1,' ',ml:1,
                        ' ',confused:1,' ',cur_reprod:1);
                      encrypt(out_rec);
                      writeln(f1,out_rec,error:=continue);
                      i1 := nptr;
                    end;
                until (i1 = 0);
 
                { Save the town level stores            }
              writev(out_rec,town_seed:12);
              encrypt(out_rec);
              writeln(f1,out_rec,error:=continue);
              for i1 := 1 to max_stores do
                with store[i1] do
                  begin
                { Save items...                 }
                    writev(out_rec,store_ctr:1);                 
                    encrypt(out_rec);
                    writeln(f1,out_rec,error:=continue);
                    for i2 := 1 to store_ctr do
                      with store_inven[i2].sitem do
                        begin
                          writev(out_rec,store_inven[i2].scost);
                          encrypt(out_rec);
                          writeln(f1,out_rec,error:=continue);
                          writev(out_rec,tchar,name);
                          encrypt(out_rec);
                          writeln(f1,out_rec,error:=continue);
                          writev(out_rec,damage);
                          encrypt(out_rec);
                          writeln(f1,out_rec,error:=continue);
                          writev(out_rec,tval:1,' ',subval:1,' ',weight:1,
                                ' ',number:1,' ',tohit:1,' ',todam:1,' ',
                                ac:1,' ',toac:1,' ',p1:1,' ',flags:1,' ',
                                level:1,' ',cost:1);
                          encrypt(out_rec);
                          writeln(f1,out_rec,error:=continue);
                        end;

                    writev(out_rec,owner:1,' ',insult_cur:1,' ',store_open:1);
                    encrypt(out_rec);
                    writeln(f1,out_rec,error:=continue);
                  end;
            end;
            close(f1,error:=continue)
          end;
        if (flag) then
          begin
            writev(out_rec,'Character saved. [Save Version ',
                                                cur_version:5:2,']');
            prt(out_rec,1,1);
            exit;
          end;
        save_char := flag;                     
        seed := get_seed;
      end;
 
 
        { Restore a saved game                          -RAK- & -JWT-   }
[psect(save$code)] function get_char(fnam : vtype) : boolean;
      var
        tot_treasures,tot_monsters : integer;
        i1,i2,i3,i4,dummy,mwk	   : integer;
        xfloor,save_seed	   : unsigned;
        save_version		   : real;
        in_rec,temp		   : ntype;
        temp_id			   : packed array [1..70] of char;
        f1			   : text;
        f2			   : file of key_type;
        dun_flag		   : boolean;
 

      {Stat conversion from 18(200) system to integer system. -RLG}
      {Kinda like converting Farenheit to Celsius}
      function new_stat(statnum : integer) : integer;
        begin
	  if (statnum < 19) then
	    new_stat := statnum
	  else if (statnum < 68) then
	    new_stat := 19
	  else if (statnum < 93) then
	    new_stat := (statnum+32) div 5
	  else if (statnum < 108) then
	    new_stat := (statnum-18) div 3
	  else if (statnum < 118) then
	    new_stat := (statnum-48) div 2
	  else if (statnum < 218) then
	    new_stat := (statnum+127) div 7
	  else
	    new_stat := 50;
	end;


      BEGIN
        dun_flag := false;
        clear(1,1);
        open (f1,FILE_NAME:=fnam,record_length:=1024,ERROR:=continue,
                HISTORY:=OLD);
        if (status(f1) <> 0) then
          begin
            prt('Error Opening> '+fnam,1,1);
            prt('',2,1);
            exit;
          end;
        reset(f1,ERROR:=continue);
        readln(f1,in_rec);
        seed := encrypt_seed2;
        decrypt(in_rec);
        temp := substr(in_rec,1,12);
        readv(temp,save_seed);
        temp := substr(in_rec,14,70);
        seed := encrypt_seed1;
        encrypt(temp);
        for mwk := 1 to 70 do                {-Opusii+MWK 6/27/87}
           temp_id[mwk] := temp[mwk];
        priv_switch(1);
        open (f2,file_name:=boss_mas,
                access_method:=keyed,organization:=indexed,
                history:=old,sharing:=readwrite,error:=continue);
        if (status(f2) <> 0) then
          begin
            prt('ERROR opening file MASTER.',1,1);
            exit;
          end;
        findk(f2,0,temp_id,eql,error:=continue);
        delete(f2,error:=continue);
        if (status(f2) <> 0) then  
          data_exception;          
        close(f2);
        priv_switch(0);
        seed := save_seed;
        prt('Restoring Character...',1,1);
        put_qio;
        readln(f1,in_rec);
        decrypt(in_rec);
        readv(in_rec,save_version);
        if (save_version <> cur_version) then
          begin
            prt('Save file is incompatable with this version.',2,1);
            writev(in_rec,'  [Save file version ',save_version:5:2,']');
            prt(in_rec,3,1);
            writev(in_rec,'  [New Save file version ',cur_version:5:2,']');
            prt(in_rec,4,1);
            if (save_version > 1.90) then
              begin
                prt('Attempting to update character for newer version...',5,1);
                dun_flag := true;
              end
            else
              exit;
            pause(24);
          end;
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,name);
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,race);
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,sex);
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,tclass);
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,title);
 
        for i1 := 1 to 5 do
          begin
            readln(f1,in_rec);
            decrypt(in_rec);
            py.misc.history[i1] := in_rec;
          end;
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
	  if (save_version < 2.40) then   { urghgh!   blegga!  }
	    begin
	      writeln('processing the pclass');
              readv(in_rec,char_row,char_col,pclass,prace,age,ht,wt,sc,
                    max_exp,exp,lev,max_lev,expfact);
	      case pclass of
	      2: pskill := 1;
	      3: pskill := 3;
	      5: pskill := 2;
	      6: pskill := 4;
	      7: pskill := 5;
	      8: pskill := 6;
	      9: pskill := 7;
	      otherwise pskill := 0;
	      end;
	    end
	  else
	    readv(in_rec,char_row,char_col,pclass,prace,pskill,age,ht,wt,sc,
                  max_exp,exp,lev,max_lev,expfact);

	readln(f1,in_rec);
        decrypt(in_rec);
        with py.misc do
          readv(in_rec,srh,perc,stl,bth,bthb,mana,cmana,mhp,chp,au,
                ptohit,ptodam,pac,ptoac,dis_th,dis_td,dis_ac,dis_tac,
                disarm,wierd,save,hitdie); 

        if (save_version < 2.10) then
	  with py.misc do
	    perc := (27 - perc)*3;

        readln(f1,in_rec);
        decrypt(in_rec);
	if (save_version < 2.30) then
          readv(in_rec,inven_ctr,inven_weight,equip_ctr,dun_level,
                missle_ctr,mon_tot_mult,turn,day,town_num,randes_seed)
	else
          readv(in_rec,inven_ctr,inven_weight,equip_ctr,dun_level,missle_ctr,
	  	 mon_tot_mult,turn,day,town_num,defeated_badguy,randes_seed);

	for i1 := 1 to 400 do
	  begin
	    readln(f1,in_rec);
	    decrypt(in_rec);
	    readv(in_rec,kill_list[i1]);	      
	  end;

	  {Read in Bank Variables}
        readln(f1,in_rec);
        decrypt(in_rec);
	readv(in_rec,act_bal,principal,payment_count,iod_val,iol_val,
		danny_cut);

        { Read in the inventory records.  }
        for i1 := 1 to inven_ctr do
          begin
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,inventory[i1].tchar,inventory[i1].name);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,inventory[i1].damage);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            with inventory[i1] do
              readv(in_rec,tval,subval,weight,number,tohit,todam,ac,
                        toac,p1,flags,level,cost);
          end;
 
        { Read in the equipment records. }
        for i1 := 23 to equip_max do
          begin
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,equipment[i1].tchar,equipment[i1].name);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,equipment[i1].damage);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            with equipment[i1] do
              readv(in_rec,tval,subval,weight,number,tohit,todam,ac,
                        toac,p1,flags,level,cost);
          end;
 
        readln(f1,in_rec);
        decrypt(in_rec);
        with py.stat do
          readv(in_rec,str,cstr,dex,cdex,con,ccon,int,cint,wis,cwis,chr,cchr);
 
	if (save_version < 2.00) then  {New stat conversion.}
	  with py.stat do
	    begin
	      str := new_stat( str);
	     cstr := new_stat(cstr);
	      int := new_stat( int);
	     cint := new_stat(cint);
   	      wis := new_stat( wis);
	     cwis := new_stat(cwis);
	      dex := new_stat( dex);
	     cdex := new_stat(cdex);
	      con := new_stat( con);
	     ccon := new_stat(ccon);
	      chr := new_stat( chr);
	     cchr := new_stat(cchr);
  	    end;
 
        with py.flags do
          begin
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,status,blind,confused,food,food_digested,protection,
                        speed,afraid,poisoned,see_inv);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,fast,slow,protevil,teleport,free_act,slow_digest);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,aggravate,sustain_str,sustain_int,sustain_wis,
                        sustain_con,sustain_dex,sustain_chr);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,fire_resist,cold_resist,acid_resist,regenerate,
                        lght_resist,ffall,confuse_monster);
 
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,image,invuln,hero,shero,blessed,resist_heat,
                         resist_cold,ring_blast,special,petrified,diseased,
	 		 detect_inv,word_recall,see_infra,tim_infra);
          end;
 
 	  if (save_version < 2.40) then   { grumble, grumble... }
	    with py.misc do
	      begin
	        writeln('about to get skill data');
	        get_skill_data(pskill);  { loc. datafiles.inc }
	        writeln('part one');
                for i1 := 1 to 33 do
                  begin
                    readln(f1,in_rec);
 	  	    if (pskill in [1,2,7]) then
		      with skill_list[i1] do
		        begin
                          decrypt(in_rec);
                          readv(in_rec,learned,sexp);
			  writeln(sexp);
		        end;
                  end;
	        writeln('part two');
                for i1 := 1 to 32 do
                  begin
                    readln(f1,in_rec);
		    if (pskill in [3,4]) then
		      with skill_list[i1] do
		        begin
                          decrypt(in_rec);
                          readv(in_rec,learned,sexp);
			  writeln(sexp);
		        end;
                  end;
	        writeln('part three');
                for i1 := 1 to 31 do
                  begin
                    readln(f1,in_rec);
		    if (pskill in [5,6]) then
		      with skill_list[i1] do
		        begin
                          decrypt(in_rec);
                          readv(in_rec,learned,sexp);
		        end;
                  end;
	        writeln('all done...');
	      end
        else
	  for i1 := 1 to max_skills do
	    with skill_list[i1] do
	      begin
	        readln(f1,in_rec);
	        decrypt(in_rec);
	        readv(in_rec,sname);
		readln(f1,in_rec);
		decrypt(in_rec);
		readv(in_rec, slevel,smana,sexp,sfail,learned); 
	     end;


                { Read the important dungeon info and floor     }
	    writeln('dung info');
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,cur_height,cur_width,max_panel_rows,max_panel_cols);
 
                { Restore the floor     }
	    writeln('floor part');
            for i1 := 1 to cur_height do
              begin
                readln(f1,in_rec);
                decrypt(in_rec);
                for i2 := 1 to cur_width do
                  begin
                    xfloor := ord(in_rec[i2]);
                    with cave[i1,i2] do
                      begin
                        fval := int(uand(%X'0F',xfloor));
                        if (uand(%X'10',xfloor) <> 0) then
                          fopen := true;
                        if (uand(%X'20',xfloor) <> 0) then
                          pl := true;
                        if (uand(%X'40',xfloor) <> 0) then
                          fm := true;
                        tl := false;
                        tptr := 0;
                        cptr := 0;
                      end;
                  end;
              end;
 
                { Restore the Treasure List             }
            tlink;
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,tot_treasures);
            for i1 := 1 to tot_treasures do
              begin
                popt(i2);
                with t_list[i2] do
                  begin
                    readln(f1,in_rec);
                    decrypt(in_rec);
                    readv(in_rec,i3,i4);
                    cave[i3,i4].tptr := i2;
 
                    readln(f1,in_rec);
                    decrypt(in_rec);
                    readv(in_rec,tchar,name);
 
                    readln(f1,in_rec);
                    decrypt(in_rec);
                    readv(in_rec,damage);
 
                    readln(f1,in_rec);
                    decrypt(in_rec);
                    readv(in_rec,tval,subval,weight,number,tohit,todam,ac,toac,
                      p1,flags,level,cost);
                  end;
              end;
 
                { Re-identify objects                   }
            readln(f1,in_rec);
            decrypt(in_rec);
            for i1 := 1 to max_objects do
              if (in_rec[i1] = 'T') then
                identify(object_list[i1])
              else
                object_ident[i1] := false;
 
                { Restore the Monster List              }
            mlink;
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,tot_monsters);
            i3 := 0;
            for i1 := 1 to tot_monsters do
              begin
                readln(f1,in_rec);
                decrypt(in_rec);
                popm(i2);
                with m_list[i2] do
                  begin
		    if (save_version < 2.20) then
		      begin
                        readv(in_rec,fy,fx,mptr,hp,cspeed,
	  	      		     csleep,cdis,ml,confused);
	                if(uand(%X'00200000',c_list[mptr].cmove) <> 0) then
			  cur_reprod := 10;
		      end
		    else
                      readv(in_rec,fy,fx,mptr,hp,cspeed,
		    	  	   csleep,cdis,ml,confused,cur_reprod);
                    cave[fy,fx].cptr := i2;
                    if (muptr = 0) then
                      muptr := i2
                    else
                      m_list[i3].nptr := i2;
                    nptr := 0;
                    i3 := i2;
                  end;
              end;
 
                { Restore the town level stores         }
            readln(f1,in_rec);
            decrypt(in_rec);
            readv(in_rec,town_seed);
            for i1 := 1 to max_stores do
              with store[i1] do
                begin
                  readln(f1,in_rec);
                  decrypt(in_rec);
                  readv(in_rec,i2);
                  store_ctr := i2;
                  for i3 := 1 to i2 do
                    with store_inven[i3].sitem do
                      begin
                        readln(f1,in_rec);
                        decrypt(in_rec);
                        readv(in_rec,store_inven[i3].scost);
                        readln(f1,in_rec);
                        decrypt(in_rec);
                        readv(in_rec,tchar,name);
                        readln(f1,in_rec);
                        decrypt(in_rec);
                        readv(in_rec,damage);
                        readln(f1,in_rec);
                        decrypt(in_rec);
                        readv(in_rec,tval,subval,weight,number,tohit,todam,
                                ac,toac,p1,flags,level,cost);
                      end;
                  readln(f1,in_rec);
                  decrypt(in_rec);
                  readv(in_rec,owner,insult_cur,store_open);
                end;
        close(f1,error:=continue);

    {Should the save file be deleted when the character is restored?}
       {open (f1,file_name:=fnam,
                record_length:=1024,history:=old,disposition:=delete,
                error:=continue);
        close(f1,error:=continue);}

        seed := get_seed;
        get_char:=dun_flag;  {If get_char is true, then moria_flag is set true}
	if (dun_flag) then
	  begin
	    store_init;
	    store_maint;
	  end;
	if (day >= day_limit) then 
	  out_of_time := true;
      END;
 
 
        { Wizard command for restoring character                -RAK-   }
[psect(save$code)] procedure restore_char;
      var
        i1                                      : integer;
        fnam                                    : vtype;
        in_rec,temp                             : ntype;
        temp_id                                 : packed array [1..70] of char;
        f1                                      : text;
        f2                                      : file of key_type;
        flag                                    : boolean;
 
      BEGIN
        prt('Name of file to be restored: ',1,1);
        if (get_string(fnam,1,30,48)) then
          begin
            priv_switch(1);
            open (f1,file_name:=fnam,
                record_length:=1024,history:=old,error:=continue);
            if (status(f1) <> 0) then
              msg_print('Error Opening> '+fnam)
            else
              begin
                flag := true;
                open (f2,file_name:=boss_mas,
                        access_method:=keyed,organization:=indexed,
                        history:=old,sharing:=readwrite,error:=continue);
                if (status(f2) <> 0) then
                  begin
                    open (f2,file_name:=boss_mas,
                        access_method:=keyed,organization:=indexed,
                        history:=new,sharing:=readwrite,error:=continue);
                    if (status(f2) <> 0) then
                      begin
                        msg_print('MASTER could not be opened.');
                        flag := false;
                      end;
                  end;
                if (flag) then
                  begin
                    reset(f1,error:=continue);
                    readln(f1,in_rec,error:=continue);
                    seed := encrypt_seed2;
                    decrypt(in_rec);
                    temp := substr(in_rec,14,70);
                    seed := encrypt_seed1;
                    encrypt(temp);
                     for i1 := 1 to 70 do
                      key_rec.file_id[i1] := temp[i1];
                    temp_id := '     ';
                    findk(f2,0,temp_id,eql,error:=continue);
                    delete(f2,error:=continue);
                    f2^ := key_rec;
                    put(f2,error:=continue);
                    if (status(f2) = 0) then
                      msg_print('Character restored...')
                    else
                      msg_print('Could not write ID in MASTER.');
                  end;
                close(f1,error:=continue);
                close(f2,error:=continue);
              end;
            seed := get_seed;
            priv_switch(0);
          end;
      END;
