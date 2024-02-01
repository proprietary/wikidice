'use client';

import React from 'react';
import Downshift from 'downshift';
import { search } from '../api';

type AutocompleteProps = {
    placeholder: string;
    buttonText: string;
    onSearch?: (query: string) => Promise<Array<string>>;
    onSubmit: (query: string) => Promise<void>;
};

type AutocompleteState = {
    query: string;
    items: Array<string>;
};

export class Autocomplete extends React.Component<AutocompleteProps> {
    public state: AutocompleteState;

    constructor(props: AutocompleteProps) {
        super(props);
        this.state = {
            query: '',
            items: [],
        };
    }

    componentDidMount() {
        this.state = {
            query: '',
            items: [],
        };
    }

    render() {
        return (
            <div>
                <Downshift
                    onStateChange={({ inputValue }) => {
                        if (inputValue != null)
                            (this.props.onSearch || search)(inputValue).then((items) => {
                                this.setState({ items });
                            });
                    }}
                    onChange={selection => this.setState({ query: selection || '' })}
                    itemToString={(item) => (item == null ? '' : item)}
                >
                    {({
                        getInputProps,
                        getItemProps,
                        getLabelProps,
                        getMenuProps,
                        isOpen,
                        highlightedIndex,
                        selectedItem,
                    }) => (
                        <div>
                            <label {...getLabelProps()}>Search</label>
                            <input
                                {...getInputProps({
                                    placeholder: "Physics",
                                    value: this.state.query,
                                    onChange: (e) => { this.setState({ query: (e.target as HTMLInputElement).value }); },
                                })}
                            />
                            <button onClick={() => this.props.onSubmit(this.state.query)}>{this.props.buttonText}</button>
                            <ul {...getMenuProps()}>
                                {isOpen
                                    ? this.state.items
                                        .map((item, index) => (
                                            <li
                                                {...getItemProps({
                                                    key: item,
                                                    index,
                                                    item,
                                                    style: {
                                                        backgroundColor:
                                                            highlightedIndex === index ? 'lightgray' : 'white',
                                                        fontWeight: selectedItem === item ? 'bold' : 'normal',
                                                    },
                                                })}
                                            >
                                                {item}
                                            </li>
                                        ))
                                    : null}
                            </ul>
                        </div>
                    )}
                </Downshift>
            </div>
        );
    }
}