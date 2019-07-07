package com.rtmapper.myapplication;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import com.gslam.svar.Svar;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);

        Svar a[]={new Svar(1),new Svar()};
        Svar svar=new Svar(a);
        svar.add(new Svar(false));
        svar.add(new Svar(1.));
        svar.add(new Svar("hello"));

        Svar buildin=Svar.instance().get("__builtin__");
        Svar json=buildin.get("Json");
        Svar var=json.call("load","{\"a\":1}");

        Svar benchClass=Svar.instance().get("BenchClass");
        Svar inst=benchClass.call(3);
        Svar getV=inst.call("get",2);
        svar.add(getV);

        Svar map=new Svar();
        map.set("vec",svar);
        map.set("hello",new Svar("hello world"));
        map.set("global",Svar.instance());
        map.set("json",json);
        map.set("result",inst);
        map.set("builtin",buildin);
        map.set("int2string",new Svar(1).call("__add__",1));
        tv.setText(map.toString());
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

}
